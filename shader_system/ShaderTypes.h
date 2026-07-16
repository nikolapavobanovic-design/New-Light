#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

// ---------------------------------------------------------------------------
// Shader stage enumeration
// ---------------------------------------------------------------------------
enum class ShaderStage {
    Vertex,
    Pixel,
    Compute,
    Geometry,
    TessControl,   // Hull shader (DX) / Tessellation Control (GL/VK)
    TessEval,      // Domain shader (DX) / Tessellation Evaluation (GL/VK)
    Count
};

// ---------------------------------------------------------------------------
// Graphics API target
// ---------------------------------------------------------------------------
enum class GraphicsAPI {
    DirectX11,
    DirectX12,
    OpenGL,
    Vulkan,
    Metal
};

// ---------------------------------------------------------------------------
// Compilation result
// ---------------------------------------------------------------------------
enum class CompileStatus {
    Success,
    Error,
    NotSupported
};

struct CompileResult {
    CompileStatus   status  = CompileStatus::Error;
    std::string     errorMessage;
    std::vector<uint8_t> bytecode;  // Compiled bytecode / SPIR-V words
};

// ---------------------------------------------------------------------------
// Shader reflection data (populated after successful compilation)
// ---------------------------------------------------------------------------
struct ConstantBufferBinding {
    std::string name;
    uint32_t    bindPoint  = 0;
    uint32_t    size       = 0;  // bytes
};

struct TextureBinding {
    std::string name;
    uint32_t    bindPoint = 0;
    ShaderStage stage     = ShaderStage::Pixel;
};

struct ShaderReflection {
    std::vector<ConstantBufferBinding> constantBuffers;
    std::vector<TextureBinding>        textures;
    uint32_t                           inputAttributeMask = 0;  // bitfield of VS inputs
};

// ---------------------------------------------------------------------------
// CPU-side shader program descriptor
// ---------------------------------------------------------------------------
struct ShaderProgramCPU {
    // Source file paths (empty = stage not used)
    std::string vsPath;   // Vertex shader
    std::string psPath;   // Pixel / Fragment shader
    std::string csPath;   // Compute shader
    std::string gsPath;   // Geometry shader
    std::string tcsPath;  // Tessellation Control / Hull
    std::string tesPath;  // Tessellation Evaluation / Domain

    // Preprocessor defines: key -> value  ("USE_NORMAL_MAP" -> "1")
    std::unordered_map<std::string, std::string> defines;

    // Optimisation level (0 = debug, 3 = max)
    int optimizationLevel = 2;

    // Enable hot-reload file watching
    bool enableHotReload = false;

    // Optional entry-point overrides (defaults applied by compiler)
    std::string vsEntry = "VSMain";
    std::string psEntry = "PSMain";
    std::string csEntry = "CSMain";
    std::string gsEntry = "GSMain";
    std::string tcsEntry = "HSMain";
    std::string tesEntry = "DSMain";
};

// ---------------------------------------------------------------------------
// Compiled shader program (result handed back to the user)
// ---------------------------------------------------------------------------
struct CompiledShaderProgram {
    // Per-stage bytecode (only populated for stages that were compiled)
    std::unordered_map<ShaderStage, std::vector<uint8_t>> stageBytecode;

    // Reflection data per stage
    std::unordered_map<ShaderStage, ShaderReflection> stageReflection;

    // Cache key used by ShaderManager
    std::string cacheKey;

    bool isValid() const { return !stageBytecode.empty(); }
};
