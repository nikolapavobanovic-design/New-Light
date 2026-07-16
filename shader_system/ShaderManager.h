#pragma once

#include "ShaderTypes.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ShaderSystem {

class IShaderCompiler;

// ---------------------------------------------------------------------------
// ShaderManager – central shader compilation, caching, and hot-reload hub
// ---------------------------------------------------------------------------
class ShaderManager {
public:
    // @param api         Target graphics API used throughout the manager's lifetime.
    // @param cacheDir    Optional directory for persistent on-disk shader cache.
    //                    Pass an empty string to disable disk caching.
    explicit ShaderManager(GraphicsAPI api, std::string cacheDir = "");
    ~ShaderManager();

    // Non-copyable, non-movable (std::mutex is not movable)
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    // -----------------------------------------------------------------------
    // Compilation
    // -----------------------------------------------------------------------

    // Compile (or retrieve from cache) a full shader program.
    // Returns nullptr on fatal error.
    std::shared_ptr<ShaderProgram> compile(const ShaderProgramCPU& desc);

    // Compile multiple variants derived from a single base descriptor.
    // Each inner vector supplies extra defines added on top of desc.defines.
    std::vector<std::shared_ptr<ShaderProgram>> compileVariants(
        const ShaderProgramCPU& base,
        const std::vector<std::vector<std::string>>& variantDefines);

    // -----------------------------------------------------------------------
    // Hot-reload
    // -----------------------------------------------------------------------

    // Callback invoked when a watched source file changes and a shader
    // has been recompiled.  The argument is the cache key of the program.
    using ReloadCallback = std::function<void(const std::string& cacheKey)>;

    // Register a callback to be invoked on hot-reload events.
    void setReloadCallback(ReloadCallback cb);

    // Poll for file-system changes; call this once per frame.
    // Recompiles any programs whose source files have been modified and
    // invokes the registered reload callback for each one.
    void update();

    // -----------------------------------------------------------------------
    // Cache management
    // -----------------------------------------------------------------------

    // Remove all in-memory cached programs.
    void clearMemoryCache();

    // Remove all cached entries whose source files no longer exist.
    void evictStaleEntries();

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    const ShaderStatistics& getStatistics() const { return m_stats; }
    void resetStatistics();

private:
    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    // Build a unique cache key from a descriptor.
    std::string buildCacheKey(const ShaderProgramCPU& desc) const;

    // Read source from disk; returns empty string on failure.
    static std::string readFile(const std::string& path);

    // Compile a single stage; returns result with bytecode.
    CompileResult compileStage(
        const std::string& source,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        bool debugInfo,
        int  optLevel);

    // Try to load a compiled program from the disk cache.
    std::shared_ptr<ShaderProgram> loadFromDiskCache(const std::string& key) const;

    // Persist a compiled program to the disk cache.
    void saveToDiskCache(const std::string& key, const ShaderProgram& prog) const;

    // Record the last-modified timestamp for all source files in desc.
    void watchSources(const std::string& key, const ShaderProgramCPU& desc);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------
    GraphicsAPI                    m_api;
    std::string                    m_cacheDir;
    std::unique_ptr<IShaderCompiler> m_compiler;

    mutable std::mutex             m_cacheMutex;
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> m_memCache;

    // Hot-reload: maps cache key → descriptor + per-file modification times
    struct WatchEntry {
        ShaderProgramCPU                          desc;
        std::unordered_map<std::string, int64_t>  fileTimes;  // path → mtime
    };
    std::unordered_map<std::string, WatchEntry> m_watched;
    ReloadCallback                               m_reloadCallback;

    ShaderStatistics m_stats;
};

} // namespace ShaderSystem
