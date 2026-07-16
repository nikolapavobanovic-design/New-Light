# Shader Management System

A production-ready, platform-agnostic shader management system for C++ graphics applications. Supports DirectX 11/12, OpenGL, Vulkan, and Metal through a unified API with caching, hot-reload, and shader variant generation.

---

## File Structure

```
shader_system/
├── ShaderTypes.h/cpp          # Core types and enumerations
├── ShaderCompiler.h/cpp       # Platform-specific compilers
├── ShaderManager.h/cpp        # Central management system
├── Example.cpp                # 10 usage examples
├── CMakeLists.txt             # Build configuration
├── README.md                  # This file
└── example_shaders/
    ├── basic.vs.hlsl          # Simple vertex shader
    ├── basic.ps.hlsl          # Simple pixel shader
    └── pbr.vs.hlsl            # PBR vertex shader with variant support
```

---

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
./ShaderExample        # Run the examples
```

---

## Quick Start

```cpp
#include "ShaderManager.h"

// 1. Create manager for your target API.
ShaderManager manager(GraphicsAPI::DirectX11);

// 2. Describe your shader program.
ShaderProgramCPU desc;
desc.vsPath = "shaders/my.vs.hlsl";
desc.psPath = "shaders/my.ps.hlsl";

// 3. Compile (or retrieve from cache).
auto shader = manager.compile(desc);
if (!shader) { /* handle error */ }

// 4. Use the bytecode with your graphics API.
auto& vsBytecode = shader->stageBytecode[ShaderStage::Vertex];
auto& psBytecode = shader->stageBytecode[ShaderStage::Pixel];
```

---

## Features

### Automatic Two-Tier Caching

Compiled shaders are cached in memory (instant lookup) and on disk (persistent across runs). The cache key is a deterministic hash of all descriptor fields.

```cpp
auto s1 = manager.compile(desc);  // miss  -> compile + cache
auto s2 = manager.compile(desc);  // hit   -> <1 ms
```

### Preprocessor Defines

```cpp
desc.defines = {
    {"MAX_LIGHTS",     "4"},
    {"USE_NORMAL_MAP", "1"}
};
auto shader = manager.compile(desc);
```

### Shader Variants

Compile multiple permutations of one base descriptor in a single call.

```cpp
ShaderProgramCPU base;
base.vsPath = "pbr.vs.hlsl";
base.psPath = "pbr.ps.hlsl";

auto shaders = manager.compileVariants(base, {
    {},                       // Base
    {"SKINNED"},              // Skinned meshes
    {"INSTANCED"},            // GPU instancing
    {"SKINNED", "INSTANCED"} // Both
});
```

### Hot-Reload

```cpp
desc.enableHotReload = true;

manager.setReloadCallback([](const std::string& key) {
    std::cout << "Reloaded: " << key << "\n";
    // Recreate GPU resources here.
});

while (running) {
    manager.update();  // Checks file timestamps; fires callback on changes.
    render();
}
```

### Statistics

```cpp
const auto& s = manager.getStatistics();
std::cout << "Compilations: " << s.compilationCount  << "\n";
std::cout << "Cache hits:   " << s.cacheHits         << "\n";
std::cout << "Avg time:     " << s.averageCompileTime << " ms\n";
```

### All Pipeline Stages

```cpp
desc.vsPath  = "mesh.vs.hlsl";
desc.gsPath  = "mesh.gs.hlsl";
desc.psPath  = "mesh.ps.hlsl";
desc.tcsPath = "mesh.hs.hlsl";   // Hull / Tessellation Control
desc.tesPath = "mesh.ds.hlsl";   // Domain / Tessellation Evaluation
desc.csPath  = "particles.cs.hlsl";
```

---

## Integrating with Graphics APIs

The system returns raw bytecode. Use it directly with your API:

### DirectX 11
```cpp
auto& bc = shader->stageBytecode[ShaderStage::Vertex];
ID3D11VertexShader* vs;
device->CreateVertexShader(bc.data(), bc.size(), nullptr, &vs);
```

### DirectX 12
```cpp
D3D12_SHADER_BYTECODE vsBC;
vsBC.pShaderBytecode = bc.data();
vsBC.BytecodeLength  = bc.size();
```

### Vulkan
```cpp
VkShaderModuleCreateInfo info{};
info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
info.codeSize = bc.size();
info.pCode    = reinterpret_cast<const uint32_t*>(bc.data());
vkCreateShaderModule(device, &info, nullptr, &module);
```

### OpenGL
```cpp
// GLSL source is returned as text bytes; upload with glShaderSource.
GLuint shader = glCreateShader(GL_VERTEX_SHADER);
const char* src = reinterpret_cast<const char*>(bc.data());
GLint len = static_cast<GLint>(bc.size());
glShaderSource(shader, 1, &src, &len);
glCompileShader(shader);
```

---

## Production Notes

The implementation ships with **mock compilers** for portability. Replace the mock `compile()` bodies in `ShaderCompiler.cpp` with real compiler calls:

| Target | Library / Tool |
|---|---|
| DirectX 11/12 | `D3DCompiler.lib` → `D3DCompile()` or `DXC` |
| OpenGL | `glslang` |
| Vulkan | `glslang` + SPIRV-Tools, or `shaderc` |
| Metal | `MTLDevice.newLibraryWithSource:options:error:` |

The architecture ensures only the `compile()` method in each compiler class needs changing.

---

## Thread Safety

- `ShaderManager::compile()` is thread-safe; the memory cache is protected by a mutex.
- `ShaderManager::update()` is **not** intended to be called concurrently with itself; call it from one thread (e.g. the main/render thread) each frame.
