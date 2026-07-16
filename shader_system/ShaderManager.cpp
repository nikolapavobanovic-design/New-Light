#include "ShaderManager.h"
#include "ShaderCompiler.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <sys/stat.h>

namespace ShaderSystem {

// ---------------------------------------------------------------------------
// Internal utility: get file modification time as Unix epoch (seconds).
// Returns -1 on failure.
// ---------------------------------------------------------------------------
static int64_t fileModTime(const std::string& path) {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) return -1;
#if defined(_WIN32)
    return static_cast<int64_t>(st.st_mtime);
#else
    return static_cast<int64_t>(st.st_mtime);
#endif
}

// ---------------------------------------------------------------------------
// A very small, non-cryptographic hash used to build cache keys.
// ---------------------------------------------------------------------------
static uint64_t fnv1a(const std::string& s) {
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : s) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// ---------------------------------------------------------------------------
// ShaderManager – constructor / destructor
// ---------------------------------------------------------------------------

ShaderManager::ShaderManager(GraphicsAPI api, std::string cacheDir)
    : m_api(api)
    , m_cacheDir(std::move(cacheDir))
    , m_compiler(ShaderCompilerFactory::create(api))
{
    if (!m_cacheDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(m_cacheDir, ec);
        if (ec) {
            throw std::runtime_error("ShaderManager: failed to create cache directory '" +
                                     m_cacheDir + "': " + ec.message());
        }
    }
}

ShaderManager::~ShaderManager() = default;

// ---------------------------------------------------------------------------
// buildCacheKey
// ---------------------------------------------------------------------------
std::string ShaderManager::buildCacheKey(const ShaderProgramCPU& desc) const {
    std::ostringstream oss;
    oss << static_cast<int>(m_api) << "|"
        << desc.vsPath  << "|" << desc.vsEntry  << "|"
        << desc.psPath  << "|" << desc.psEntry  << "|"
        << desc.csPath  << "|" << desc.csEntry  << "|"
        << desc.gsPath  << "|" << desc.gsEntry  << "|"
        << desc.tcsPath << "|" << desc.tcsEntry << "|"
        << desc.tesPath << "|" << desc.tesEntry << "|"
        << desc.optimizationLevel << "|"
        << (desc.debugInfo ? "dbg" : "rel");

    // Include defines in a deterministic order
    std::vector<std::pair<std::string,std::string>> sortedDefs(
        desc.defines.begin(), desc.defines.end());
    std::sort(sortedDefs.begin(), sortedDefs.end());
    for (auto& [k, v] : sortedDefs) {
        oss << "|" << k << "=" << v;
    }

    uint64_t h = fnv1a(oss.str());
    std::ostringstream keyOut;
    keyOut << std::hex << h;
    return keyOut.str();
}

// ---------------------------------------------------------------------------
// readFile
// ---------------------------------------------------------------------------
std::string ShaderManager::readFile(const std::string& path) {
    std::ifstream ifs(path, std::ios::in | std::ios::binary);
    if (!ifs.is_open()) return {};
    return {std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
}

// ---------------------------------------------------------------------------
// compileStage
// ---------------------------------------------------------------------------
CompileResult ShaderManager::compileStage(
    const std::string& source,
    ShaderStage        stage,
    const std::string& entryPoint,
    const std::unordered_map<std::string, std::string>& defines,
    bool debugInfo,
    int  optLevel)
{
    return m_compiler->compile(source, stage, entryPoint, defines, debugInfo, optLevel);
}

// ---------------------------------------------------------------------------
// Disk cache (simple binary serialisation)
// ---------------------------------------------------------------------------

static std::string diskCachePath(const std::string& dir, const std::string& key) {
    return dir + "/" + key + ".shcache";
}

// Very small serialiser: [uint32 numStages] per stage: [uint8 stageId][uint32 bcSize][bytes...]
std::shared_ptr<ShaderProgram> ShaderManager::loadFromDiskCache(const std::string& key) const {
    if (m_cacheDir.empty()) return nullptr;

    std::ifstream ifs(diskCachePath(m_cacheDir, key), std::ios::binary);
    if (!ifs.is_open()) return nullptr;

    auto prog = std::make_shared<ShaderProgram>();
    prog->cacheKey = key;
    prog->status   = CompileStatus::Success;

    uint32_t numStages = 0;
    ifs.read(reinterpret_cast<char*>(&numStages), sizeof(numStages));
    if (ifs.fail()) return nullptr;

    for (uint32_t i = 0; i < numStages; ++i) {
        uint8_t stageId = 0;
        uint32_t bcSize = 0;
        ifs.read(reinterpret_cast<char*>(&stageId), 1);
        ifs.read(reinterpret_cast<char*>(&bcSize), sizeof(bcSize));
        if (ifs.fail()) return nullptr;

        Bytecode bc(bcSize);
        ifs.read(reinterpret_cast<char*>(bc.data()), bcSize);
        if (ifs.fail()) return nullptr;

        prog->stageBytecode[static_cast<ShaderStage>(stageId)] = std::move(bc);
    }
    return prog;
}

void ShaderManager::saveToDiskCache(const std::string& key, const ShaderProgram& prog) const {
    if (m_cacheDir.empty()) return;

    std::ofstream ofs(diskCachePath(m_cacheDir, key), std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) return;

    auto numStages = static_cast<uint32_t>(prog.stageBytecode.size());
    ofs.write(reinterpret_cast<const char*>(&numStages), sizeof(numStages));

    for (auto& [stage, bc] : prog.stageBytecode) {
        auto stageId = static_cast<uint8_t>(stage);
        auto bcSize  = static_cast<uint32_t>(bc.size());
        ofs.write(reinterpret_cast<const char*>(&stageId), 1);
        ofs.write(reinterpret_cast<const char*>(&bcSize), sizeof(bcSize));
        ofs.write(reinterpret_cast<const char*>(bc.data()), bcSize);
    }
}

// ---------------------------------------------------------------------------
// watchSources
// ---------------------------------------------------------------------------
void ShaderManager::watchSources(const std::string& key, const ShaderProgramCPU& desc) {
    WatchEntry entry;
    entry.desc = desc;

    auto trackFile = [&](const std::string& path) {
        if (!path.empty()) {
            entry.fileTimes[path] = fileModTime(path);
        }
    };
    trackFile(desc.vsPath);
    trackFile(desc.psPath);
    trackFile(desc.csPath);
    trackFile(desc.gsPath);
    trackFile(desc.tcsPath);
    trackFile(desc.tesPath);

    m_watched[key] = std::move(entry);
}

// ---------------------------------------------------------------------------
// compile
// ---------------------------------------------------------------------------
std::shared_ptr<ShaderProgram> ShaderManager::compile(const ShaderProgramCPU& desc) {
    const std::string key = buildCacheKey(desc);

    // 1. Memory cache hit
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_memCache.find(key);
        if (it != m_memCache.end()) {
            ++m_stats.cacheHits;
            return it->second;
        }
    }

    // 2. Disk cache hit
    if (auto prog = loadFromDiskCache(key)) {
        prog->cacheKey = key;
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_memCache[key] = prog;
        }
        ++m_stats.cacheHits;
        if (desc.enableHotReload) watchSources(key, desc);
        return prog;
    }

    ++m_stats.cacheMisses;

    // 3. Actual compilation
    auto t0 = std::chrono::high_resolution_clock::now();

    auto prog = std::make_shared<ShaderProgram>();
    prog->cacheKey = key;
    prog->status   = CompileStatus::Success;

    struct StageInfo {
        std::string path;
        ShaderStage stage;
        std::string entry;
    };

    const std::vector<StageInfo> stages = {
        {desc.vsPath,  ShaderStage::Vertex,      desc.vsEntry},
        {desc.psPath,  ShaderStage::Pixel,       desc.psEntry},
        {desc.csPath,  ShaderStage::Compute,     desc.csEntry},
        {desc.gsPath,  ShaderStage::Geometry,    desc.gsEntry},
        {desc.tcsPath, ShaderStage::TessControl, desc.tcsEntry},
        {desc.tesPath, ShaderStage::TessEval,    desc.tesEntry},
    };

    for (auto& si : stages) {
        if (si.path.empty()) continue;

        std::string source = readFile(si.path);
        if (source.empty()) {
            CompileMessage msg;
            msg.status  = CompileStatus::Error;
            msg.message = "Failed to read source file: " + si.path;
            msg.file    = si.path;
            prog->stageMessages[si.stage].push_back(msg);
            prog->status = CompileStatus::Error;
            ++m_stats.errorCount;
            continue;
        }

        CompileResult res = compileStage(
            source, si.stage, si.entry, desc.defines, desc.debugInfo, desc.optimizationLevel);

        prog->stageMessages[si.stage] = res.messages;
        prog->totalCompileTimeMs     += res.compileTimeMs;

        if (res.status == CompileStatus::Error) {
            prog->status = CompileStatus::Error;
            ++m_stats.errorCount;
        } else {
            prog->stageBytecode[si.stage] = std::move(res.bytecode);
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    prog->totalCompileTimeMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    ++m_stats.compilationCount;
    m_stats.totalCompileTimeMs += prog->totalCompileTimeMs;
    if (m_stats.compilationCount > 0) {
        m_stats.averageCompileTime =
            m_stats.totalCompileTimeMs / static_cast<double>(m_stats.compilationCount);
    }

    if (prog->isValid()) {
        {
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_memCache[key] = prog;
        }
        saveToDiskCache(key, *prog);
    }

    if (desc.enableHotReload) watchSources(key, desc);

    return prog;
}

// ---------------------------------------------------------------------------
// compileVariants
// ---------------------------------------------------------------------------
std::vector<std::shared_ptr<ShaderProgram>> ShaderManager::compileVariants(
    const ShaderProgramCPU& base,
    const std::vector<std::vector<std::string>>& variantDefines)
{
    std::vector<std::shared_ptr<ShaderProgram>> results;
    results.reserve(variantDefines.size());

    for (auto& extraDefines : variantDefines) {
        ShaderProgramCPU desc = base;
        for (auto& def : extraDefines) {
            desc.defines[def] = "1";
        }
        results.push_back(compile(desc));
    }
    return results;
}

// ---------------------------------------------------------------------------
// Hot-reload: update()
// ---------------------------------------------------------------------------
void ShaderManager::update() {
    for (auto& [key, entry] : m_watched) {
        bool changed = false;
        for (auto& [path, oldTime] : entry.fileTimes) {
            int64_t newTime = fileModTime(path);
            if (newTime != oldTime && newTime != -1) {
                changed  = true;
                oldTime  = newTime;
            }
        }

        if (changed) {
            // Invalidate memory cache and recompile
            {
                std::lock_guard<std::mutex> lock(m_cacheMutex);
                m_memCache.erase(key);
            }

            auto prog = compile(entry.desc);
            if (prog && prog->isValid()) {
                ++m_stats.hotReloadCount;
                if (m_reloadCallback) {
                    m_reloadCallback(key);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// setReloadCallback
// ---------------------------------------------------------------------------
void ShaderManager::setReloadCallback(ReloadCallback cb) {
    m_reloadCallback = std::move(cb);
}

// ---------------------------------------------------------------------------
// Cache management
// ---------------------------------------------------------------------------
void ShaderManager::clearMemoryCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_memCache.clear();
}

void ShaderManager::evictStaleEntries() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    for (auto it = m_memCache.begin(); it != m_memCache.end(); ) {
        // Check if the watched entry still has valid files
        auto wit = m_watched.find(it->first);
        if (wit != m_watched.end()) {
            bool anyMissing = false;
            for (auto& [path, mtime] : wit->second.fileTimes) {
                (void)mtime;
                if (fileModTime(path) == -1) { anyMissing = true; break; }
            }
            if (anyMissing) { it = m_memCache.erase(it); continue; }
        }
        ++it;
    }
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------
void ShaderManager::resetStatistics() {
    m_stats = ShaderStatistics{};
}

} // namespace ShaderSystem
