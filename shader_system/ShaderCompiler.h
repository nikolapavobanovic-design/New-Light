#pragma once

#include "ShaderTypes.h"
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Abstract base – one subclass per graphics API / shader language
// ---------------------------------------------------------------------------
class IShaderCompiler {
public:
    virtual ~IShaderCompiler() = default;

    // Compile a single shader stage from source file.
    // @param sourcePath  Path to the HLSL / GLSL source file.
    // @param stage       Which pipeline stage to compile.
    // @param entryPoint  Entry-point function name.
    // @param defines     Preprocessor macro definitions.
    // @param optLevel    Optimisation level (0-3).
    virtual CompileResult compile(
        const std::string& sourcePath,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        int                optLevel) = 0;

    // Return the API this compiler targets.
    virtual GraphicsAPI targetAPI() const = 0;
};

// ---------------------------------------------------------------------------
// DirectX 11 / 12 compiler (D3DCompile / DXC)
// ---------------------------------------------------------------------------
class DirectXShaderCompiler : public IShaderCompiler {
public:
    explicit DirectXShaderCompiler(GraphicsAPI api);

    CompileResult compile(
        const std::string& sourcePath,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        int                optLevel) override;

    GraphicsAPI targetAPI() const override { return m_api; }

private:
    GraphicsAPI m_api;

    // Returns the shader model string, e.g. "vs_5_0" for DX11 vertex.
    std::string shaderModel(ShaderStage stage) const;
};

// ---------------------------------------------------------------------------
// OpenGL GLSL compiler (wraps glslang)
// ---------------------------------------------------------------------------
class GLSLShaderCompiler : public IShaderCompiler {
public:
    GLSLShaderCompiler();

    CompileResult compile(
        const std::string& sourcePath,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        int                optLevel) override;

    GraphicsAPI targetAPI() const override { return GraphicsAPI::OpenGL; }
};

// ---------------------------------------------------------------------------
// Vulkan SPIR-V compiler (glslang + SPIR-V tools)
// ---------------------------------------------------------------------------
class VulkanShaderCompiler : public IShaderCompiler {
public:
    VulkanShaderCompiler();

    CompileResult compile(
        const std::string& sourcePath,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        int                optLevel) override;

    GraphicsAPI targetAPI() const override { return GraphicsAPI::Vulkan; }
};

// ---------------------------------------------------------------------------
// Metal MSL compiler
// ---------------------------------------------------------------------------
class MetalShaderCompiler : public IShaderCompiler {
public:
    MetalShaderCompiler();

    CompileResult compile(
        const std::string& sourcePath,
        ShaderStage        stage,
        const std::string& entryPoint,
        const std::unordered_map<std::string, std::string>& defines,
        int                optLevel) override;

    GraphicsAPI targetAPI() const override { return GraphicsAPI::Metal; }
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
class ShaderCompilerFactory {
public:
    // Create the appropriate compiler for the given API.
    static std::unique_ptr<IShaderCompiler> create(GraphicsAPI api);
};
