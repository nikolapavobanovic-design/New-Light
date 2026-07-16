#include "ShaderCompiler.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

// Read an entire text file into a string. Returns empty string on failure.
static std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// Build a preamble string of #define directives.
static std::string buildPreamble(
    const std::unordered_map<std::string, std::string>& defines)
{
    std::string preamble;
    for (const auto& kv : defines) {
        preamble += "#define " + kv.first;
        if (!kv.second.empty()) {
            preamble += " " + kv.second;
        }
        preamble += "\n";
    }
    return preamble;
}

// Placeholder: produce mock bytecode (a copy of source bytes).
// In production replace with real compiler invocations.
static std::vector<uint8_t> makeMockBytecode(const std::string& source,
                                              ShaderStage stage,
                                              const std::string& entry) {
    // Tag the mock output so tests can verify per-stage content.
    std::string tagged = "[stage:" + std::to_string(static_cast<int>(stage)) +
                         "|entry:" + entry + "]\n" + source;
    return std::vector<uint8_t>(tagged.begin(), tagged.end());
}

} // namespace

// ===========================================================================
// DirectXShaderCompiler
// ===========================================================================
DirectXShaderCompiler::DirectXShaderCompiler(GraphicsAPI api) : m_api(api) {}

std::string DirectXShaderCompiler::shaderModel(ShaderStage stage) const {
    const bool isDX12 = (m_api == GraphicsAPI::DirectX12);
    const std::string ver = isDX12 ? "5_1" : "5_0";

    switch (stage) {
        case ShaderStage::Vertex:      return "vs_" + ver;
        case ShaderStage::Pixel:       return "ps_" + ver;
        case ShaderStage::Compute:     return "cs_" + ver;
        case ShaderStage::Geometry:    return "gs_" + ver;
        case ShaderStage::TessControl: return "hs_" + ver;
        case ShaderStage::TessEval:    return "ds_" + ver;
        default:                       return "vs_" + ver;
    }
}

CompileResult DirectXShaderCompiler::compile(
    const std::string& sourcePath,
    ShaderStage        stage,
    const std::string& entryPoint,
    const std::unordered_map<std::string, std::string>& defines,
    int                /*optLevel*/)
{
    std::string source = readFile(sourcePath);
    if (source.empty()) {
        return { CompileStatus::Error, "Cannot open file: " + sourcePath, {} };
    }

    std::string fullSource = buildPreamble(defines) + source;

    // -----------------------------------------------------------------
    // Production integration point:
    //   HRESULT hr = D3DCompile(fullSource.c_str(), fullSource.size(),
    //       sourcePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
    //       entryPoint.c_str(), shaderModel(stage).c_str(),
    //       flags, 0, &blob, &errBlob);
    // -----------------------------------------------------------------

    CompileResult result;
    result.status   = CompileStatus::Success;
    result.bytecode = makeMockBytecode(fullSource, stage, entryPoint);
    return result;
}

// ===========================================================================
// GLSLShaderCompiler
// ===========================================================================
GLSLShaderCompiler::GLSLShaderCompiler() {}

CompileResult GLSLShaderCompiler::compile(
    const std::string& sourcePath,
    ShaderStage        stage,
    const std::string& entryPoint,
    const std::unordered_map<std::string, std::string>& defines,
    int                /*optLevel*/)
{
    std::string source = readFile(sourcePath);
    if (source.empty()) {
        return { CompileStatus::Error, "Cannot open file: " + sourcePath, {} };
    }

    std::string fullSource = "#version 450 core\n" + buildPreamble(defines) + source;

    // -----------------------------------------------------------------
    // Production integration point:
    //   glslang::InitializeProcess();
    //   glslang::TShader shader(eeqStage);
    //   shader.setStrings(...);
    //   shader.parse(...);
    // -----------------------------------------------------------------

    CompileResult result;
    result.status   = CompileStatus::Success;
    result.bytecode = makeMockBytecode(fullSource, stage, entryPoint);
    return result;
}

// ===========================================================================
// VulkanShaderCompiler
// ===========================================================================
VulkanShaderCompiler::VulkanShaderCompiler() {}

CompileResult VulkanShaderCompiler::compile(
    const std::string& sourcePath,
    ShaderStage        stage,
    const std::string& entryPoint,
    const std::unordered_map<std::string, std::string>& defines,
    int                /*optLevel*/)
{
    std::string source = readFile(sourcePath);
    if (source.empty()) {
        return { CompileStatus::Error, "Cannot open file: " + sourcePath, {} };
    }

    std::string fullSource = "#version 450\n" + buildPreamble(defines) + source;

    // -----------------------------------------------------------------
    // Production integration point:
    //   Use glslang + SPIRV-Tools to produce SPIR-V words, or invoke
    //   glslangValidator / shaderc as a subprocess / library.
    // -----------------------------------------------------------------

    CompileResult result;
    result.status   = CompileStatus::Success;
    result.bytecode = makeMockBytecode(fullSource, stage, entryPoint);
    return result;
}

// ===========================================================================
// MetalShaderCompiler
// ===========================================================================
MetalShaderCompiler::MetalShaderCompiler() {}

CompileResult MetalShaderCompiler::compile(
    const std::string& sourcePath,
    ShaderStage        stage,
    const std::string& entryPoint,
    const std::unordered_map<std::string, std::string>& defines,
    int                /*optLevel*/)
{
    std::string source = readFile(sourcePath);
    if (source.empty()) {
        return { CompileStatus::Error, "Cannot open file: " + sourcePath, {} };
    }

    std::string fullSource = buildPreamble(defines) + source;

    // -----------------------------------------------------------------
    // Production integration point:
    //   Use MTLDevice newLibraryWithSource:options:error: on Apple platforms.
    // -----------------------------------------------------------------

    CompileResult result;
    result.status   = CompileStatus::Success;
    result.bytecode = makeMockBytecode(fullSource, stage, entryPoint);
    return result;
}

// ===========================================================================
// ShaderCompilerFactory
// ===========================================================================
std::unique_ptr<IShaderCompiler> ShaderCompilerFactory::create(GraphicsAPI api) {
    switch (api) {
        case GraphicsAPI::DirectX11:
        case GraphicsAPI::DirectX12:
            return std::make_unique<DirectXShaderCompiler>(api);
        case GraphicsAPI::OpenGL:
            return std::make_unique<GLSLShaderCompiler>();
        case GraphicsAPI::Vulkan:
            return std::make_unique<VulkanShaderCompiler>();
        case GraphicsAPI::Metal:
            return std::make_unique<MetalShaderCompiler>();
        default:
            throw std::runtime_error("Unknown GraphicsAPI");
    }
}
