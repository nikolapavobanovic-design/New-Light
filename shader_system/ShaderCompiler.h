#pragma once

#include "ShaderTypes.h"
#include <memory>
#include <string>

namespace ShaderSystem {

// ---------------------------------------------------------------------------
// IShaderCompiler – pure interface every backend must implement
// ---------------------------------------------------------------------------
class IShaderCompiler {
public:
    virtual ~IShaderCompiler() = default;

    // Compile a single shader stage from source text.
    // @param source      HLSL / GLSL source code
    // @param stage       Target shader stage
    // @param entryPoint  Name of the entry-point function
    // @param defines     Preprocessor definitions (name -> value)
    // @param debugInfo   Whether to embed debug symbols
    // @param optLevel    Optimisation level 0–3
    virtual CompileResult compile(
        const std::string& source,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        bool debugInfo,
        int  optLevel) = 0;

    // Compiler name for diagnostic messages.
    virtual std::string name() const = 0;
};

// ---------------------------------------------------------------------------
// DirectXCompiler – wraps D3DCompile / DXC (mock implementation)
// ---------------------------------------------------------------------------
class DirectXCompiler : public IShaderCompiler {
public:
    explicit DirectXCompiler(bool useDXC = false);

    CompileResult compile(
        const std::string& source,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        bool debugInfo,
        int  optLevel) override;

    std::string name() const override;

private:
    bool m_useDXC;

    // Maps ShaderStage to HLSL profile string, e.g. "vs_5_0"
    static std::string hlslProfile(ShaderStage stage, bool useDXC);
};

// ---------------------------------------------------------------------------
// GLSLCompiler – wraps glslang (mock implementation)
// ---------------------------------------------------------------------------
class GLSLCompiler : public IShaderCompiler {
public:
    CompileResult compile(
        const std::string& source,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        bool debugInfo,
        int  optLevel) override;

    std::string name() const override { return "GLSLCompiler (glslang)"; }
};

// ---------------------------------------------------------------------------
// SPIRVCompiler – cross-compiler targeting SPIR-V (mock implementation)
// ---------------------------------------------------------------------------
class SPIRVCompiler : public IShaderCompiler {
public:
    CompileResult compile(
        const std::string& source,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        bool debugInfo,
        int  optLevel) override;

    std::string name() const override { return "SPIRVCompiler"; }
};

// ---------------------------------------------------------------------------
// Factory – creates the correct compiler for the requested API
// ---------------------------------------------------------------------------
class ShaderCompilerFactory {
public:
    // Returns a compiler suitable for the given graphics API.
    static std::unique_ptr<IShaderCompiler> create(GraphicsAPI api);
};

} // namespace ShaderSystem
