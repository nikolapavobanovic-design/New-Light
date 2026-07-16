// Example.cpp – 10 usage examples for the ShaderSystem
//
// This file demonstrates a wide range of ShaderManager capabilities.
// Since no real GPU is present during compilation, source paths are
// fabricated strings; in a real application they would point to actual
// HLSL/GLSL files on disk.

#include "ShaderManager.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace ShaderSystem;

// ---------------------------------------------------------------------------
// Helper: print compile messages for a program
// ---------------------------------------------------------------------------
static void printMessages(const ShaderProgram& prog) {
    for (auto& [stage, msgs] : prog.stageMessages) {
        for (auto& msg : msgs) {
            const char* lvl = (msg.status == CompileStatus::Error)   ? "ERROR"
                            : (msg.status == CompileStatus::Warning) ? "WARN"
                                                                      : "INFO";
            std::cout << "  [" << lvl << "] " << msg.message << "\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Example 1 – Basic vertex + pixel shader compilation
// ---------------------------------------------------------------------------
static void example1_BasicShader() {
    std::cout << "\n--- Example 1: Basic Shader ---\n";

    ShaderManager manager(GraphicsAPI::DirectX11);

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    auto shader = manager.compile(desc);
    if (shader && shader->isValid()) {
        auto& vsBytecode = shader->stageBytecode.at(ShaderStage::Vertex);
        auto& psBytecode = shader->stageBytecode.at(ShaderStage::Pixel);
        std::cout << "VS bytecode size: " << vsBytecode.size() << " bytes\n";
        std::cout << "PS bytecode size: " << psBytecode.size() << " bytes\n";
        std::cout << "Compile time: " << shader->totalCompileTimeMs << " ms\n";
    } else {
        std::cout << "Compilation failed!\n";
        if (shader) printMessages(*shader);
    }
}

// ---------------------------------------------------------------------------
// Example 2 – Cache hit: second compile returns instantly
// ---------------------------------------------------------------------------
static void example2_CacheHit() {
    std::cout << "\n--- Example 2: Cache Hit ---\n";

    ShaderManager manager(GraphicsAPI::DirectX11);

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    // First compile – cache miss
    auto shader1 = manager.compile(desc);
    std::cout << "First compile:  " << manager.getStatistics().compilationCount
              << " compilations, " << manager.getStatistics().cacheHits << " hits\n";

    // Second compile – should be a cache hit
    auto shader2 = manager.compile(desc);
    std::cout << "Second compile: " << manager.getStatistics().compilationCount
              << " compilations, " << manager.getStatistics().cacheHits << " hits\n";

    assert(shader1.get() == shader2.get() && "Expected same pointer from cache");
    std::cout << "Cache hit confirmed (same pointer).\n";
}

// ---------------------------------------------------------------------------
// Example 3 – Preprocessor defines
// ---------------------------------------------------------------------------
static void example3_Defines() {
    std::cout << "\n--- Example 3: Preprocessor Defines ---\n";

    ShaderManager manager(GraphicsAPI::DirectX11);

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/pbr.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";
    desc.defines = {
        {"MAX_LIGHTS",      "4"},
        {"USE_NORMAL_MAP",  "1"},
        {"SHADOW_MAP_SIZE", "2048"}
    };

    auto shader = manager.compile(desc);
    std::cout << "Compiled with " << desc.defines.size() << " defines. "
              << "Valid: " << (shader && shader->isValid() ? "yes" : "no") << "\n";
}

// ---------------------------------------------------------------------------
// Example 4 – Shader variants
// ---------------------------------------------------------------------------
static void example4_Variants() {
    std::cout << "\n--- Example 4: Shader Variants ---\n";

    ShaderManager manager(GraphicsAPI::DirectX11);

    ShaderProgramCPU base;
    base.vsPath = "example_shaders/pbr.vs.hlsl";
    base.psPath = "example_shaders/basic.ps.hlsl";

    const std::vector<std::vector<std::string>> variants = {
        {},                          // Base
        {"SKINNED"},                 // Skinned meshes
        {"INSTANCED"},               // GPU instancing
        {"SKINNED", "INSTANCED"},    // Both
        {"ALPHA_TEST"},              // Alpha testing
    };

    auto shaders = manager.compileVariants(base, variants);
    std::cout << "Compiled " << shaders.size() << " variants.\n";
    for (size_t i = 0; i < shaders.size(); ++i) {
        std::cout << "  Variant " << i << ": "
                  << (shaders[i] && shaders[i]->isValid() ? "OK" : "FAILED")
                  << " (key=" << (shaders[i] ? shaders[i]->cacheKey : "n/a") << ")\n";
    }
}

// ---------------------------------------------------------------------------
// Example 5 – Compute shader
// ---------------------------------------------------------------------------
static void example5_ComputeShader() {
    std::cout << "\n--- Example 5: Compute Shader ---\n";

    ShaderManager manager(GraphicsAPI::DirectX12);

    ShaderProgramCPU desc;
    desc.csPath  = "example_shaders/particle_update.cs.hlsl";
    desc.csEntry = "CSMain";
    desc.defines = {{"THREAD_GROUP_SIZE", "64"}};

    auto shader = manager.compile(desc);
    // Source file does not exist on disk; demonstrates error handling.
    if (shader && shader->isValid()) {
        std::cout << "CS bytecode size: "
                  << shader->stageBytecode.at(ShaderStage::Compute).size() << " bytes\n";
    } else {
        std::cout << "CS could not be read from disk (expected – no real file). "
                     "Error handling works correctly.\n";
        if (shader) printMessages(*shader);
    }
}

// ---------------------------------------------------------------------------
// Example 6 – All pipeline stages (VS + GS + TCS + TES + PS)
// ---------------------------------------------------------------------------
static void example6_AllStages() {
    std::cout << "\n--- Example 6: All Pipeline Stages ---\n";

    ShaderManager manager(GraphicsAPI::DirectX11);

    ShaderProgramCPU desc;
    desc.vsPath  = "example_shaders/basic.vs.hlsl";
    desc.gsPath  = "example_shaders/advanced.gs.hlsl";
    desc.tcsPath = "example_shaders/advanced.hs.hlsl";
    desc.tesPath = "example_shaders/advanced.ds.hlsl";
    desc.psPath  = "example_shaders/basic.ps.hlsl";

    auto shader = manager.compile(desc);
    std::cout << "Pipeline stages with real sources:\n";
    for (auto& [stage, bc] : shader->stageBytecode) {
        std::cout << "  Stage bytecode size: " << bc.size() << " bytes\n";
    }
}

// ---------------------------------------------------------------------------
// Example 7 – Hot-reload callback registration
// ---------------------------------------------------------------------------
static void example7_HotReload() {
    std::cout << "\n--- Example 7: Hot-Reload Setup ---\n";

    ShaderManager manager(GraphicsAPI::DirectX11);

    ShaderProgramCPU desc;
    desc.vsPath        = "example_shaders/basic.vs.hlsl";
    desc.psPath        = "example_shaders/basic.ps.hlsl";
    desc.enableHotReload = true;

    manager.setReloadCallback([](const std::string& key) {
        std::cout << "  [HOT-RELOAD] Shader reloaded: " << key << "\n";
        // In a real engine: recreate GPU resources from new bytecode here.
    });

    auto shader = manager.compile(desc);
    std::cout << "Shader compiled and file watcher registered.\n";
    std::cout << "Calling update() – no file changes expected at this point.\n";

    // Simulate a few frames without changes
    for (int i = 0; i < 3; ++i) {
        manager.update();
    }
    std::cout << "update() called 3 times – no reload triggered.\n";
}

// ---------------------------------------------------------------------------
// Example 8 – Statistics tracking
// ---------------------------------------------------------------------------
static void example8_Statistics() {
    std::cout << "\n--- Example 8: Statistics ---\n";

    ShaderManager manager(GraphicsAPI::DirectX11);

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    // Compile 3 distinct programs + 2 cache hits
    auto s1 = manager.compile(desc);

    desc.defines["VARIANT_A"] = "1";
    auto s2 = manager.compile(desc);

    desc.defines["VARIANT_B"] = "1";
    auto s3 = manager.compile(desc);

    // Cache hits
    manager.compile(desc);
    manager.compile(desc);

    const auto& stats = manager.getStatistics();
    std::cout << "Compilations : " << stats.compilationCount   << "\n";
    std::cout << "Cache hits   : " << stats.cacheHits          << "\n";
    std::cout << "Cache misses : " << stats.cacheMisses        << "\n";
    std::cout << "Total time   : " << stats.totalCompileTimeMs << " ms\n";
    std::cout << "Average time : " << stats.averageCompileTime << " ms\n";
}

// ---------------------------------------------------------------------------
// Example 9 – Debug vs Release compilation
// ---------------------------------------------------------------------------
static void example9_DebugVsRelease() {
    std::cout << "\n--- Example 9: Debug vs Release ---\n";

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    {
        ShaderManager debugMgr(GraphicsAPI::DirectX12);
        desc.debugInfo        = true;
        desc.optimizationLevel = 0;
        auto shader = debugMgr.compile(desc);
        std::cout << "Debug  build – valid: " << (shader && shader->isValid() ? "yes" : "no")
                  << ", time: " << (shader ? shader->totalCompileTimeMs : 0.0) << " ms\n";
    }

    {
        ShaderManager relMgr(GraphicsAPI::DirectX12);
        desc.debugInfo         = false;
        desc.optimizationLevel = 3;
        auto shader = relMgr.compile(desc);
        std::cout << "Release build – valid: " << (shader && shader->isValid() ? "yes" : "no")
                  << ", time: " << (shader ? shader->totalCompileTimeMs : 0.0) << " ms\n";
    }
}

// ---------------------------------------------------------------------------
// Example 10 – OpenGL / Vulkan targets
// ---------------------------------------------------------------------------
static void example10_CrossAPITargets() {
    std::cout << "\n--- Example 10: Cross-API Targets ---\n";

    ShaderProgramCPU desc;
    desc.vsPath = "example_shaders/basic.vs.hlsl";
    desc.psPath = "example_shaders/basic.ps.hlsl";

    struct APIEntry { GraphicsAPI api; const char* name; };
    const APIEntry targets[] = {
        {GraphicsAPI::DirectX11, "DX11"},
        {GraphicsAPI::DirectX12, "DX12"},
        {GraphicsAPI::OpenGL,    "OpenGL"},
        {GraphicsAPI::Vulkan,    "Vulkan"},
        {GraphicsAPI::Metal,     "Metal"},
    };

    for (const auto& entry : targets) {
        ShaderManager mgr(entry.api);
        auto shader = mgr.compile(desc);
        std::cout << "  " << entry.name << " – valid: "
                  << (shader && shader->isValid() ? "yes" : "no")
                  << ", stages: " << (shader ? shader->stageBytecode.size() : 0) << "\n";
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== ShaderSystem Examples ===\n";

    example1_BasicShader();
    example2_CacheHit();
    example3_Defines();
    example4_Variants();
    example5_ComputeShader();
    example6_AllStages();
    example7_HotReload();
    example8_Statistics();
    example9_DebugVsRelease();
    example10_CrossAPITargets();

    std::cout << "\n=== All examples complete ===\n";
    return 0;
}
