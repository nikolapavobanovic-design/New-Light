#pragma once

#include "ShaderTypes.h"
#include "ShaderCompiler.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <chrono>
#include <atomic>

// ---------------------------------------------------------------------------
// Statistics collected by ShaderManager
// ---------------------------------------------------------------------------
struct ShaderManagerStats {
    std::atomic<uint64_t> compilationCount  {0};
    std::atomic<uint64_t> cacheHits         {0};
    std::atomic<uint64_t> cacheMisses       {0};
    std::atomic<uint64_t> hotReloadCount    {0};
    double                averageCompileTime{0.0};  // milliseconds

    // Non-copyable due to atomic members; provide manual copy helper.
    ShaderManagerStats() = default;
    ShaderManagerStats(const ShaderManagerStats&) = delete;
    ShaderManagerStats& operator=(const ShaderManagerStats&) = delete;
};

// ---------------------------------------------------------------------------
// Main shader management system
// ---------------------------------------------------------------------------
class ShaderManager {
public:
    // Callback invoked when a shader is hot-reloaded.
    // Receives the cache key of the invalidated program.
    using ReloadCallback = std::function<void(const std::string& cacheKey)>;

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------
    explicit ShaderManager(GraphicsAPI api,
                           const std::string& diskCacheDir = "shader_cache");
    ~ShaderManager();

    // Non-copyable, non-movable (owns mutexes and atomics).
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    // -----------------------------------------------------------------------
    // Core compilation
    // -----------------------------------------------------------------------

    // Compile (or retrieve from cache) a shader program described by @desc.
    // Returns nullptr on failure.
    std::shared_ptr<CompiledShaderProgram> compile(const ShaderProgramCPU& desc);

    // Compile multiple variant permutations of a base descriptor.
    // @param base      Base descriptor (paths, opt level, hot-reload setting).
    // @param variantDefines  Each entry is a list of define names (value "1").
    // Returns one result per variant; failed slots hold nullptr.
    std::vector<std::shared_ptr<CompiledShaderProgram>> compileVariants(
        const ShaderProgramCPU&                     base,
        const std::vector<std::vector<std::string>>& variantDefines);

    // -----------------------------------------------------------------------
    // Hot-reload
    // -----------------------------------------------------------------------

    // Register a callback that fires when any watched shader is reloaded.
    void setReloadCallback(ReloadCallback cb);

    // Poll for file-system changes and recompile stale shaders.
    // Call once per frame from your game / render loop.
    void update();

    // -----------------------------------------------------------------------
    // Cache management
    // -----------------------------------------------------------------------

    // Evict all in-memory cached entries.
    void clearMemoryCache();

    // Delete all files in the disk cache directory.
    void clearDiskCache();

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    const ShaderManagerStats& getStatistics() const { return m_stats; }

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    // Build a deterministic cache key from a descriptor.
    static std::string buildCacheKey(const ShaderProgramCPU& desc);

private:
    // Per-file watch record for hot-reload.
    struct WatchEntry {
        std::string                            path;
        std::chrono::system_clock::time_point  lastModified;
        std::vector<std::string>               dependentKeys;  // cache keys
    };

    // -----------------------------------------------------------------------
    // Internals
    // -----------------------------------------------------------------------
    std::shared_ptr<CompiledShaderProgram> compileInternal(
        const ShaderProgramCPU& desc,
        const std::string&      cacheKey);

    // Compile a single stage; returns empty vector on failure.
    std::vector<uint8_t> compileStage(
        const std::string& sourcePath,
        ShaderStage        stage,
        const std::string& entryPoint,
        const ShaderProgramCPU& desc,
        std::string&       outError);

    // Disk-cache helpers.
    bool       saveToDisk(const std::string& key,
                          const CompiledShaderProgram& program) const;
    std::shared_ptr<CompiledShaderProgram>
               loadFromDisk(const std::string& key) const;
    std::string diskCachePath(const std::string& key) const;

    // File-watching helpers.
    void registerWatch(const std::string& path, const std::string& cacheKey);
    std::chrono::system_clock::time_point getLastModified(
        const std::string& path) const;

    // Update running average compile time.
    void recordCompileTime(double ms);

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------
    GraphicsAPI                            m_api;
    std::string                            m_diskCacheDir;
    std::unique_ptr<IShaderCompiler>       m_compiler;

    // Memory cache: key -> compiled program
    mutable std::mutex                     m_cacheMutex;
    std::unordered_map<std::string,
        std::shared_ptr<CompiledShaderProgram>> m_memoryCache;

    // Hot-reload
    std::mutex                             m_watchMutex;
    std::unordered_map<std::string, WatchEntry> m_watchEntries; // path -> entry
    ReloadCallback                         m_reloadCallback;

    // Statistics
    ShaderManagerStats                     m_stats;
    std::mutex                             m_statsMutex;
    double                                 m_totalCompileTime{0.0};
};
