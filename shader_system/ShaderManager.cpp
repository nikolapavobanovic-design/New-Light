#include "ShaderManager.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Cache-key utilities
// ---------------------------------------------------------------------------
namespace {

// A simple FNV-1a hash over a string.
static uint64_t fnv1a(const std::string& s) {
    uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= static_cast<uint64_t>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

// Build a canonical string that uniquely identifies a shader descriptor.
static std::string descriptorString(const ShaderProgramCPU& desc) {
    std::ostringstream ss;
    ss << "vs=" << desc.vsPath
       << "|ps=" << desc.psPath
       << "|cs=" << desc.csPath
       << "|gs=" << desc.gsPath
       << "|tcs=" << desc.tcsPath
       << "|tes=" << desc.tesPath
       << "|opt=" << desc.optimizationLevel
       << "|vse=" << desc.vsEntry
       << "|pse=" << desc.psEntry
       << "|cse=" << desc.csEntry
       << "|gse=" << desc.gsEntry
       << "|tcse=" << desc.tcsEntry
       << "|tese=" << desc.tesEntry;

    // Sort defines for determinism.
    std::vector<std::pair<std::string,std::string>>
        sorted(desc.defines.begin(), desc.defines.end());
    std::sort(sorted.begin(), sorted.end());
    for (const auto& kv : sorted) {
        ss << "|def=" << kv.first << "=" << kv.second;
    }
    return ss.str();
}

} // namespace

// ===========================================================================
// Construction / destruction
// ===========================================================================
ShaderManager::ShaderManager(GraphicsAPI api, const std::string& diskCacheDir)
    : m_api(api)
    , m_diskCacheDir(diskCacheDir)
    , m_compiler(ShaderCompilerFactory::create(api))
{
    // Create disk cache directory if it does not exist.
    std::error_code ec;
    fs::create_directories(m_diskCacheDir, ec);
    if (ec) {
        std::cerr << "[ShaderManager] Warning: could not create cache dir '"
                  << m_diskCacheDir << "': " << ec.message() << "\n";
    }
}

ShaderManager::~ShaderManager() = default;

// ===========================================================================
// Public API – buildCacheKey
// ===========================================================================
std::string ShaderManager::buildCacheKey(const ShaderProgramCPU& desc) {
    const std::string raw = descriptorString(desc);
    const uint64_t    h   = fnv1a(raw);

    std::ostringstream ss;
    ss << std::hex << h;
    return ss.str();
}

// ===========================================================================
// Public API – compile
// ===========================================================================
std::shared_ptr<CompiledShaderProgram>
ShaderManager::compile(const ShaderProgramCPU& desc) {
    const std::string key = buildCacheKey(desc);

    // 1. Check memory cache (fast path).
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_memoryCache.find(key);
        if (it != m_memoryCache.end()) {
            ++m_stats.cacheHits;
            return it->second;
        }
    }

    // 2. Check disk cache.
    {
        auto diskResult = loadFromDisk(key);
        if (diskResult) {
            ++m_stats.cacheHits;
            std::lock_guard<std::mutex> lock(m_cacheMutex);
            m_memoryCache[key] = diskResult;
            return diskResult;
        }
    }

    // 3. Compile from source.
    ++m_stats.cacheMisses;
    return compileInternal(desc, key);
}

// ===========================================================================
// Public API – compileVariants
// ===========================================================================
std::vector<std::shared_ptr<CompiledShaderProgram>>
ShaderManager::compileVariants(
    const ShaderProgramCPU&                      base,
    const std::vector<std::vector<std::string>>& variantDefines)
{
    std::vector<std::shared_ptr<CompiledShaderProgram>> results;
    results.reserve(variantDefines.size());

    for (const auto& names : variantDefines) {
        ShaderProgramCPU desc = base;
        for (const auto& name : names) {
            desc.defines[name] = "1";
        }
        results.push_back(compile(desc));
    }

    return results;
}

// ===========================================================================
// Hot-reload
// ===========================================================================
void ShaderManager::setReloadCallback(ReloadCallback cb) {
    std::lock_guard<std::mutex> lock(m_watchMutex);
    m_reloadCallback = std::move(cb);
}

void ShaderManager::update() {
    std::vector<std::pair<std::string, std::string>> stale; // (path, key)

    {
        std::lock_guard<std::mutex> lock(m_watchMutex);
        for (auto& [path, entry] : m_watchEntries) {
            auto current = getLastModified(path);
            if (current > entry.lastModified) {
                entry.lastModified = current;
                for (const auto& key : entry.dependentKeys) {
                    stale.emplace_back(path, key);
                }
            }
        }
    }

    if (stale.empty()) return;

    // Evict stale entries from memory cache.
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        for (const auto& [path, key] : stale) {
            m_memoryCache.erase(key);
        }
    }

    // Fire callback.
    ReloadCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_watchMutex);
        cb = m_reloadCallback;
    }
    if (cb) {
        for (const auto& [path, key] : stale) {
            ++m_stats.hotReloadCount;
            cb(key);
        }
    }
}

// ===========================================================================
// Cache management
// ===========================================================================
void ShaderManager::clearMemoryCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_memoryCache.clear();
}

void ShaderManager::clearDiskCache() {
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(m_diskCacheDir, ec)) {
        fs::remove(entry.path(), ec);
    }
}

// ===========================================================================
// Private – compileInternal
// ===========================================================================
std::shared_ptr<CompiledShaderProgram>
ShaderManager::compileInternal(const ShaderProgramCPU& desc,
                                const std::string&      key)
{
    auto program       = std::make_shared<CompiledShaderProgram>();
    program->cacheKey  = key;

    auto t0 = std::chrono::high_resolution_clock::now();

    // Helper: compile one stage if path is non-empty.
    auto tryCompile = [&](const std::string& path,
                          ShaderStage        stage,
                          const std::string& entry) -> bool {
        if (path.empty()) return true;  // stage not requested
        std::string err;
        auto bytecode = compileStage(path, stage, entry, desc, err);
        if (bytecode.empty()) {
            std::cerr << "[ShaderManager] Compile error in '" << path
                      << "': " << err << "\n";
            return false;
        }
        program->stageBytecode[stage] = std::move(bytecode);

        // Register watch entry for hot-reload.
        if (desc.enableHotReload) {
            registerWatch(path, key);
        }
        return true;
    };

    bool ok = true;
    ok = ok && tryCompile(desc.vsPath,  ShaderStage::Vertex,      desc.vsEntry);
    ok = ok && tryCompile(desc.psPath,  ShaderStage::Pixel,       desc.psEntry);
    ok = ok && tryCompile(desc.csPath,  ShaderStage::Compute,     desc.csEntry);
    ok = ok && tryCompile(desc.gsPath,  ShaderStage::Geometry,    desc.gsEntry);
    ok = ok && tryCompile(desc.tcsPath, ShaderStage::TessControl, desc.tcsEntry);
    ok = ok && tryCompile(desc.tesPath, ShaderStage::TessEval,    desc.tesEntry);

    if (!ok || program->stageBytecode.empty()) {
        return nullptr;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    recordCompileTime(ms);

    ++m_stats.compilationCount;

    // Store in memory cache.
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_memoryCache[key] = program;
    }

    // Persist to disk.
    saveToDisk(key, *program);

    return program;
}

// ===========================================================================
// Private – compileStage
// ===========================================================================
std::vector<uint8_t> ShaderManager::compileStage(
    const std::string& sourcePath,
    ShaderStage        stage,
    const std::string& entryPoint,
    const ShaderProgramCPU& desc,
    std::string&       outError)
{
    CompileResult result = m_compiler->compile(
        sourcePath, stage, entryPoint, desc.defines, desc.optimizationLevel);

    if (result.status != CompileStatus::Success) {
        outError = result.errorMessage;
        return {};
    }
    return std::move(result.bytecode);
}

// ===========================================================================
// Private – disk cache helpers
// ===========================================================================
std::string ShaderManager::diskCachePath(const std::string& key) const {
    return m_diskCacheDir + "/" + key + ".cache";
}

bool ShaderManager::saveToDisk(const std::string&           key,
                                const CompiledShaderProgram& program) const
{
    std::ofstream file(diskCachePath(key), std::ios::binary);
    if (!file.is_open()) return false;

    // Simple binary format:
    // [uint32 stageCount]
    //   per stage: [uint32 stageId] [uint64 byteLen] [bytes...]
    uint32_t stageCount = static_cast<uint32_t>(program.stageBytecode.size());
    file.write(reinterpret_cast<const char*>(&stageCount), sizeof(stageCount));

    for (const auto& [stage, bytecode] : program.stageBytecode) {
        uint32_t sid = static_cast<uint32_t>(stage);
        uint64_t len = static_cast<uint64_t>(bytecode.size());
        file.write(reinterpret_cast<const char*>(&sid), sizeof(sid));
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(reinterpret_cast<const char*>(bytecode.data()),
                   static_cast<std::streamsize>(len));
    }
    return file.good();
}

std::shared_ptr<CompiledShaderProgram>
ShaderManager::loadFromDisk(const std::string& key) const {
    std::ifstream file(diskCachePath(key), std::ios::binary);
    if (!file.is_open()) return nullptr;

    uint32_t stageCount = 0;
    file.read(reinterpret_cast<char*>(&stageCount), sizeof(stageCount));
    if (!file) return nullptr;

    auto program      = std::make_shared<CompiledShaderProgram>();
    program->cacheKey = key;

    for (uint32_t i = 0; i < stageCount; ++i) {
        uint32_t sid = 0;
        uint64_t len = 0;
        file.read(reinterpret_cast<char*>(&sid), sizeof(sid));
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (!file) return nullptr;

        auto stage = static_cast<ShaderStage>(sid);
        std::vector<uint8_t> bytecode(static_cast<size_t>(len));
        file.read(reinterpret_cast<char*>(bytecode.data()),
                  static_cast<std::streamsize>(len));
        if (!file) return nullptr;

        program->stageBytecode[stage] = std::move(bytecode);
    }

    return program->stageBytecode.empty() ? nullptr : program;
}

// ===========================================================================
// Private – file-watching helpers
// ===========================================================================
void ShaderManager::registerWatch(const std::string& path,
                                   const std::string& cacheKey) {
    std::lock_guard<std::mutex> lock(m_watchMutex);
    auto& entry = m_watchEntries[path];
    entry.path  = path;
    if (entry.lastModified == std::chrono::system_clock::time_point{}) {
        entry.lastModified = getLastModified(path);
    }
    // Only add if not already tracked.
    const auto& deps = entry.dependentKeys;
    if (std::find(deps.begin(), deps.end(), cacheKey) == deps.end()) {
        entry.dependentKeys.push_back(cacheKey);
    }
}

std::chrono::system_clock::time_point
ShaderManager::getLastModified(const std::string& path) const {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return {};
    // Convert file_time_type to system_clock::time_point.
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() +
        std::chrono::system_clock::now());
    return sctp;
}

// ===========================================================================
// Private – statistics helpers
// ===========================================================================
void ShaderManager::recordCompileTime(double ms) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    m_totalCompileTime += ms;
    uint64_t count = m_stats.compilationCount.load() + 1; // +1 for current
    const_cast<ShaderManagerStats&>(m_stats).averageCompileTime =
        m_totalCompileTime / static_cast<double>(count);
}
