#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace ShaderSystem {

// ---------------------------------------------------------------------------
// Shader stage enumeration
// ---------------------------------------------------------------------------
enum class ShaderStage {
    Vertex,          // VS  - vertex processing
    Pixel,           // PS  - fragment/pixel processing
    Compute,         // CS  - general-purpose compute
    Geometry,        // GS  - geometry amplification/reduction
    TessControl,     // TCS / Hull shader
    TessEval,        // TES / Domain shader
    COUNT
};

// ---------------------------------------------------------------------------
// Target graphics API
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
    Warning,
    Error,
    NotSupported
};

struct CompileMessage {
    CompileStatus   status;
    std::string     message;
    int             line   = -1;
    std::string     file;
};

// Raw compiled bytecode for a single stage
using Bytecode = std::vector<uint8_t>;

struct CompileResult {
    CompileStatus               status  = CompileStatus::Error;
    Bytecode                    bytecode;
    std::vector<CompileMessage> messages;
    double                      compileTimeMs = 0.0;
};

// ---------------------------------------------------------------------------
// Shader reflection (optional metadata extracted after compilation)
// ---------------------------------------------------------------------------
struct ConstantBufferDesc {
    std::string name;
    uint32_t    slot;
    uint32_t    sizeBytes;
};

struct TextureDesc {
    std::string name;
    uint32_t    slot;
    ShaderStage stage;
};

struct SamplerDesc {
    std::string name;
    uint32_t    slot;
    ShaderStage stage;
};

struct InputElementDesc {
    std::string semantic;
    uint32_t    semanticIndex;
    uint32_t    sizeBytes;
};

struct ShaderReflection {
    std::vector<ConstantBufferDesc> constantBuffers;
    std::vector<TextureDesc>        textures;
    std::vector<SamplerDesc>        samplers;
    std::vector<InputElementDesc>   inputLayout;  // VS only
};

// ---------------------------------------------------------------------------
// CPU-side shader program descriptor
// ---------------------------------------------------------------------------
struct ShaderProgramCPU {
    // Source file paths (empty = stage unused)
    std::string vsPath;   // Vertex shader
    std::string psPath;   // Pixel / fragment shader
    std::string csPath;   // Compute shader
    std::string gsPath;   // Geometry shader
    std::string tcsPath;  // Tessellation control / hull
    std::string tesPath;  // Tessellation evaluation / domain

    // Preprocessor defines applied to every stage
    std::unordered_map<std::string, std::string> defines;

    // Optional entry-point overrides (defaults shown below)
    std::string vsEntry  = "VSMain";
    std::string psEntry  = "PSMain";
    std::string csEntry  = "CSMain";
    std::string gsEntry  = "GSMain";
    std::string tcsEntry = "HullMain";
    std::string tesEntry = "DomainMain";

    // Feature flags
    bool enableHotReload = false;
    bool enableReflection = false;
    bool debugInfo = false;

    // Optimization level (0 = none, 3 = maximum)
    int optimizationLevel = 2;
};

// ---------------------------------------------------------------------------
// Compiled shader program (result returned to the caller)
// ---------------------------------------------------------------------------
struct ShaderProgram {
    // Bytecode for each compiled stage (empty if stage unused)
    std::unordered_map<ShaderStage, Bytecode> stageBytecode;

    // Compilation messages per stage
    std::unordered_map<ShaderStage, std::vector<CompileMessage>> stageMessages;

    // Optional reflection data per stage
    std::unordered_map<ShaderStage, ShaderReflection> stageReflection;

    // Overall compilation status
    CompileStatus status = CompileStatus::Error;

    // Wall-clock time for the whole program compilation (ms)
    double totalCompileTimeMs = 0.0;

    // Cache key used to store/retrieve this program
    std::string cacheKey;

    bool isValid() const { return status == CompileStatus::Success || status == CompileStatus::Warning; }
};

// ---------------------------------------------------------------------------
// Statistics counters
// ---------------------------------------------------------------------------
struct ShaderStatistics {
    uint64_t compilationCount   = 0;
    uint64_t cacheHits          = 0;
    uint64_t cacheMisses        = 0;
    uint64_t hotReloadCount     = 0;
    uint64_t errorCount         = 0;
    double   totalCompileTimeMs = 0.0;
    double   averageCompileTime = 0.0;  // ms per compilation
};

} // namespace ShaderSystem
