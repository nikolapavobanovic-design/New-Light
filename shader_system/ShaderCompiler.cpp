#include "ShaderCompiler.h"

#include <chrono>
#include <sstream>

namespace ShaderSystem {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Generate a trivial mock bytecode so the rest of the pipeline has something
// to work with.  In production this would be the output of D3DCompile /
// glslangValidator / SPIRV-Tools etc.
static Bytecode makeMockBytecode(ShaderStage stage, const std::string& entryPoint) {
    // Encode stage + entry-point length into the fake blob so that different
    // compilations produce distinguishably different "bytecode".
    Bytecode bc;
    bc.push_back(static_cast<uint8_t>(stage));
    bc.push_back(static_cast<uint8_t>(entryPoint.size() & 0xFF));
    for (char c : entryPoint) {
        bc.push_back(static_cast<uint8_t>(c));
    }
    // Pad to a round number
    while (bc.size() < 64) bc.push_back(0xCC);
    return bc;
}

// ---------------------------------------------------------------------------
// DirectXCompiler
// ---------------------------------------------------------------------------

DirectXCompiler::DirectXCompiler(bool useDXC) : m_useDXC(useDXC) {}

std::string DirectXCompiler::name() const {
    return m_useDXC ? "DirectXCompiler (DXC)" : "DirectXCompiler (D3DCompile)";
}

std::string DirectXCompiler::hlslProfile(ShaderStage stage, bool useDXC) {
    // DXC uses shader model 6.x; legacy D3DCompile uses 5.x.
    const char* ver = useDXC ? "6_0" : "5_0";
    switch (stage) {
        case ShaderStage::Vertex:      return std::string("vs_") + ver;
        case ShaderStage::Pixel:       return std::string("ps_") + ver;
        case ShaderStage::Compute:     return std::string("cs_") + ver;
        case ShaderStage::Geometry:    return std::string("gs_") + ver;
        case ShaderStage::TessControl: return std::string("hs_") + ver;
        case ShaderStage::TessEval:    return std::string("ds_") + ver;
        default:                       return "unknown";
    }
}

CompileResult DirectXCompiler::compile(
    const std::string& source,
    ShaderStage        stage,
    const std::string& entryPoint,
    const std::unordered_map<std::string, std::string>& defines,
    bool /*debugInfo*/,
    int  /*optLevel*/)
{
    auto t0 = std::chrono::high_resolution_clock::now();

    CompileResult result;

    if (source.empty()) {
        result.status = CompileStatus::Error;
        result.messages.push_back({CompileStatus::Error, "Empty source passed to DirectXCompiler", -1, ""});
        return result;
    }

    // --- PRODUCTION INTEGRATION POINT ---
    // Replace the lines below with a real D3DCompile() / DXC call:
    //
    //   ID3DBlob* pCode = nullptr;
    //   ID3DBlob* pErrors = nullptr;
    //   std::vector<D3D_SHADER_MACRO> macros = buildMacros(defines);
    //   HRESULT hr = D3DCompile(
    //       source.c_str(), source.size(), nullptr,
    //       macros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE,
    //       entryPoint.c_str(), hlslProfile(stage, m_useDXC).c_str(),
    //       flags, 0, &pCode, &pErrors);
    //   ...

    (void)defines; // suppress unused-variable warning in mock
    result.bytecode = makeMockBytecode(stage, entryPoint);
    result.status   = CompileStatus::Success;

    auto t1 = std::chrono::high_resolution_clock::now();
    result.compileTimeMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    return result;
}

// ---------------------------------------------------------------------------
// GLSLCompiler
// ---------------------------------------------------------------------------

CompileResult GLSLCompiler::compile(
    const std::string& source,
    ShaderStage        stage,
    const std::string& entryPoint,
    const std::unordered_map<std::string, std::string>& defines,
    bool /*debugInfo*/,
    int  /*optLevel*/)
{
    auto t0 = std::chrono::high_resolution_clock::now();

    CompileResult result;

    if (source.empty()) {
        result.status = CompileStatus::Error;
        result.messages.push_back({CompileStatus::Error, "Empty source passed to GLSLCompiler", -1, ""});
        return result;
    }

    // --- PRODUCTION INTEGRATION POINT ---
    // Replace with glslang TShader / TProgram calls:
    //
    //   glslang::TShader shader(glslStage(stage));
    //   std::string preamble = buildPreamble(defines);
    //   const char* strings[] = { preamble.c_str(), source.c_str() };
    //   shader.setStrings(strings, 2);
    //   shader.parse(&DefaultTBuiltInResource, 450, false, messages);
    //   ...

    (void)defines;
    (void)entryPoint;
    result.bytecode = makeMockBytecode(stage, "main");
    result.status   = CompileStatus::Success;

    auto t1 = std::chrono::high_resolution_clock::now();
    result.compileTimeMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    return result;
}

// ---------------------------------------------------------------------------
// SPIRVCompiler
// ---------------------------------------------------------------------------

CompileResult SPIRVCompiler::compile(
    const std::string& source,
    ShaderStage        stage,
    const std::string& entryPoint,
    const std::unordered_map<std::string, std::string>& defines,
    bool /*debugInfo*/,
    int  /*optLevel*/)
{
    auto t0 = std::chrono::high_resolution_clock::now();

    CompileResult result;

    if (source.empty()) {
        result.status = CompileStatus::Error;
        result.messages.push_back({CompileStatus::Error, "Empty source passed to SPIRVCompiler", -1, ""});
        return result;
    }

    // --- PRODUCTION INTEGRATION POINT ---
    // Use glslang + SPIRV-Tools, or shaderc:
    //
    //   shaderc::Compiler compiler;
    //   shaderc::CompileOptions options;
    //   for (auto& [k, v] : defines) options.AddMacroDefinition(k, v);
    //   auto res = compiler.CompileGlslToSpv(
    //       source, shadercKind(stage), "shader", options);
    //   ...

    (void)defines;
    (void)entryPoint;
    result.bytecode = makeMockBytecode(stage, "main");
    result.status   = CompileStatus::Success;

    auto t1 = std::chrono::high_resolution_clock::now();
    result.compileTimeMs =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    return result;
}

// ---------------------------------------------------------------------------
// ShaderCompilerFactory
// ---------------------------------------------------------------------------

std::unique_ptr<IShaderCompiler> ShaderCompilerFactory::create(GraphicsAPI api) {
    switch (api) {
        case GraphicsAPI::DirectX11:
            return std::make_unique<DirectXCompiler>(/*useDXC=*/false);
        case GraphicsAPI::DirectX12:
            return std::make_unique<DirectXCompiler>(/*useDXC=*/true);
        case GraphicsAPI::OpenGL:
            return std::make_unique<GLSLCompiler>();
        case GraphicsAPI::Vulkan:
            return std::make_unique<SPIRVCompiler>();
        case GraphicsAPI::Metal:
            // Metal uses MSL; fall back to GLSL cross-compiler for now.
            return std::make_unique<SPIRVCompiler>();
        default:
            return std::make_unique<DirectXCompiler>(false);
    }
}

} // namespace ShaderSystem
