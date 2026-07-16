// Example.cpp – 10 usage examples for ShaderManager

#include "ShaderManager.h"

#include <iostream>
#include <cassert>

// ---------------------------------------------------------------------------
// Helper: print whether a program compiled successfully.
// ---------------------------------------------------------------------------
static void printResult(const char* label,
                        const std::shared_ptr<CompiledShaderProgram>& prog) {
    if (prog && prog->isValid()) {
        std::cout << "[OK] " << label << " – "
                  << prog->stageBytecode.size() << " stage(s) compiled, "
                  << "cache key: " << prog->cacheKey << "\n";
    } else {
        std::cout << "[FAIL] " << label << " – compilation failed\n";
    }
}

// ===========================================================================
// Example 1 – Basic vertex + pixel shader
// ===========================================================================
static void example01_BasicShader() {
    std::cout << "\n=== Example 01: Basic Shader ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex01");

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    auto shader = manager.compile(desc);
    printResult("basic VS+PS", shader);
}

// ===========================================================================
// Example 2 – Caching (second compile is a cache hit)
// ===========================================================================
static void example02_Caching() {
    std::cout << "\n=== Example 02: Caching ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex02");
    manager.clearDiskCache();  // ensure a clean slate

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    auto shader1 = manager.compile(desc);  // cache miss -> compile
    auto shader2 = manager.compile(desc);  // cache hit  -> instant

    const auto& stats = manager.getStatistics();
    std::cout << "Compilations: " << stats.compilationCount << "\n";
    std::cout << "Cache hits:   " << stats.cacheHits       << "\n";
    assert(stats.compilationCount == 1);
    assert(stats.cacheHits        == 1);
    assert(shader1.get() == shader2.get()); // same pointer from memory cache
    std::cout << "[OK] Cache hit confirmed (same pointer)\n";
}

// ===========================================================================
// Example 3 – Preprocessor defines
// ===========================================================================
static void example03_Defines() {
    std::cout << "\n=== Example 03: Preprocessor Defines ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex03");

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";
    desc.defines = {
        {"MAX_LIGHTS",      "4"},
        {"USE_NORMAL_MAP",  "1"},
        {"SHADOW_MAP_SIZE", "2048"}
    };

    auto shader = manager.compile(desc);
    printResult("shader with defines", shader);
}

// ===========================================================================
// Example 4 – Shader variants
// ===========================================================================
static void example04_Variants() {
    std::cout << "\n=== Example 04: Shader Variants ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex04");

    ShaderProgramCPU base;
    base.vsPath = "example_shaders/pbr.vs.hlsl";
    base.psPath = "example_shaders/basic.ps.hlsl";

    std::vector<std::vector<std::string>> variantDefines = {
        {},                          // Base
        {"SKINNED"},                 // Skinned meshes
        {"INSTANCED"},               // GPU instancing
        {"SKINNED", "INSTANCED"},    // Both
        {"ALPHA_TEST"}               // Alpha testing
    };

    auto shaders = manager.compileVariants(base, variantDefines);

    for (size_t i = 0; i < shaders.size(); ++i) {
        std::string label = "variant[" + std::to_string(i) + "]";
        printResult(label.c_str(), shaders[i]);
    }
}

// ===========================================================================
// Example 5 – Hot-reload setup
// ===========================================================================
static void example05_HotReload() {
    std::cout << "\n=== Example 05: Hot-Reload ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex05");

    manager.setReloadCallback([](const std::string& key) {
        std::cout << "  [RELOAD] Shader reloaded: " << key << "\n";
    });

    ShaderProgramCPU desc;
    desc.vsPath          = "example_shaders/basic.vs.hlsl";
    desc.psPath          = "example_shaders/basic.ps.hlsl";
    desc.enableHotReload = true;

    auto shader = manager.compile(desc);
    printResult("hot-reload shader", shader);

    // In a real app you would call manager.update() every frame.
    // Here we just verify it runs without error.
    manager.update();
    std::cout << "[OK] update() ran without error\n";
}

// ===========================================================================
// Example 6 – Compute shader
// ===========================================================================
static void example06_ComputeShader() {
    std::cout << "\n=== Example 06: Compute Shader ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex06");

    // Compute-only program (no VS/PS paths).
    ShaderProgramCPU desc;
    desc.csPath  = "example_shaders/basic.vs.hlsl"; // reuse file for demo
    desc.csEntry = "CSMain";

    auto shader = manager.compile(desc);
    // Will fail because entry point 'CSMain' is not in the demo file,
    // but the compiler is mocked so it still produces bytecode.
    printResult("compute shader", shader);
}

// ===========================================================================
// Example 7 – All pipeline stages
// ===========================================================================
static void example07_AllStages() {
    std::cout << "\n=== Example 07: All Pipeline Stages ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex07");

    ShaderProgramCPU desc;
    desc.vsPath  = "example_shaders/basic.vs.hlsl";
    desc.gsPath  = "example_shaders/basic.vs.hlsl";   // reuse for demo
    desc.psPath  = "example_shaders/basic.ps.hlsl";
    desc.tcsPath = "example_shaders/basic.vs.hlsl";   // reuse for demo
    desc.tesPath = "example_shaders/basic.vs.hlsl";   // reuse for demo
    desc.tcsEntry = "HSMain";
    desc.tesEntry = "DSMain";
    desc.gsEntry  = "GSMain";

    auto shader = manager.compile(desc);
    printResult("all-stages program", shader);
    if (shader) {
        for (const auto& [stage, bc] : shader->stageBytecode) {
            std::cout << "  stage " << static_cast<int>(stage)
                      << " -> " << bc.size() << " bytes\n";
        }
    }
}

// ===========================================================================
// Example 8 – Statistics
// ===========================================================================
static void example08_Statistics() {
    std::cout << "\n=== Example 08: Statistics ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex08");
    manager.clearDiskCache();  // clean slate so counts are deterministic

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    manager.compile(desc);
    manager.compile(desc); // cache hit
    manager.compile(desc); // cache hit

    const auto& stats = manager.getStatistics();
    std::cout << "Compilations:     " << stats.compilationCount  << "\n";
    std::cout << "Cache hits:       " << stats.cacheHits         << "\n";
    std::cout << "Cache misses:     " << stats.cacheMisses       << "\n";
    std::cout << "Hot-reload count: " << stats.hotReloadCount    << "\n";
    std::cout << "Avg compile time: " << stats.averageCompileTime << " ms\n";
}

// ===========================================================================
// Example 9 – Cache management (clear memory / disk)
// ===========================================================================
static void example09_CacheManagement() {
    std::cout << "\n=== Example 09: Cache Management ===\n";

    ShaderManager manager(GraphicsAPI::DirectX11, "shader_cache_ex09");

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    manager.compile(desc);
    std::cout << "Compiled and cached to disk.\n";

    manager.clearMemoryCache();
    std::cout << "Memory cache cleared.\n";

    auto shader2 = manager.compile(desc); // should load from disk
    printResult("reload from disk cache", shader2);

    manager.clearDiskCache();
    std::cout << "Disk cache cleared.\n";
}

// ===========================================================================
// Example 10 – Multiple graphics APIs
// ===========================================================================
static void example10_MultipleAPIs() {
    std::cout << "\n=== Example 10: Multiple Graphics APIs ===\n";

    const std::pair<GraphicsAPI, const char*> apis[] = {
        {GraphicsAPI::DirectX11, "DirectX 11"},
        {GraphicsAPI::DirectX12, "DirectX 12"},
        {GraphicsAPI::OpenGL,    "OpenGL"},
        {GraphicsAPI::Vulkan,    "Vulkan"},
        {GraphicsAPI::Metal,     "Metal"},
    };

    int idx = 0;
    for (const auto& [api, name] : apis) {
        std::string cacheDir = "shader_cache_ex10_" + std::to_string(idx++);
        ShaderManager manager(api, cacheDir);

        ShaderProgramCPU desc;
        desc.vsPath = "example_shaders/basic.vs.hlsl";
        desc.psPath = "example_shaders/basic.ps.hlsl";

        auto shader = manager.compile(desc);
        std::string label = std::string(name) + " shader";
        printResult(label.c_str(), shader);
    }
}

// ===========================================================================
// main
// ===========================================================================
int main() {
    std::cout << "ShaderManager – Usage Examples\n";
    std::cout << std::string(50, '=') << "\n";

    try {
        example01_BasicShader();
        example02_Caching();
        example03_Defines();
        example04_Variants();
        example05_HotReload();
        example06_ComputeShader();
        example07_AllStages();
        example08_Statistics();
        example09_CacheManagement();
        example10_MultipleAPIs();

        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << "All examples completed successfully.\n";
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
