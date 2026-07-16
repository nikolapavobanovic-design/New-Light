# ShaderSystem

A production-ready, cross-platform **shader management system** for game engines and real-time graphics applications.

## Features

| Feature | Description |
|---|---|
| **Multi-API** | DirectX 11/12, OpenGL, Vulkan, Metal |
| **Two-tier cache** | In-memory (instant) + on-disk (persistent) |
| **Hot-reload** | File-change detection with a callback API |
| **Shader variants** | Compile permutation lists in one call |
| **Preprocessor defines** | Per-program macro injection |
| **Statistics** | Compilation counts, cache ratios, timings |
| **Thread-safe** | Mutex-guarded memory cache |

## File Structure

```
shader_system/
├── ShaderTypes.h/cpp       # Core types (stages, APIs, results, reflection)
├── ShaderCompiler.h/cpp    # Backend compilers (DX, GLSL, SPIR-V)
├── ShaderManager.h/cpp     # Central manager
├── Example.cpp             # 10 runnable usage examples
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
└── example_shaders/
    ├── basic.vs.hlsl       # Simple vertex shader
    ├── basic.ps.hlsl       # Simple pixel/fragment shader
    └── pbr.vs.hlsl         # Advanced PBR vertex shader (SKINNED/INSTANCED variants)
```

## Quick Start

### 1. Build

```bash
cd shader_system
mkdir build && cd build
cmake ..
cmake --build .
./ShaderExample
```

### 2. Basic Usage

```cpp
#include "ShaderManager.h"
using namespace ShaderSystem;

ShaderManager manager(GraphicsAPI::DirectX11);

ShaderProgramCPU desc;
desc.vsPath = "shaders/basic.vs.hlsl";
desc.psPath = "shaders/basic.ps.hlsl";

auto shader = manager.compile(desc);
if (shader && shader->isValid()) {
    auto& vsBytecode = shader->stageBytecode[ShaderStage::Vertex];
    // Feed bytecode to graphics API...
}
```

## Usage Examples

### Caching

```cpp
// First compile – hits the compiler.
auto shader = manager.compile(desc);

// Second compile – returns the cached pointer in < 1 ms.
auto shader2 = manager.compile(desc);
```

### Preprocessor Defines

```cpp
desc.defines = {
    {"MAX_LIGHTS",      "4"},
    {"USE_NORMAL_MAP",  "1"},
    {"SHADOW_MAP_SIZE", "2048"},
};
auto shader = manager.compile(desc);
```

### Shader Variants

```cpp
ShaderProgramCPU base;
base.vsPath = "pbr.vs.hlsl";
base.psPath = "pbr.ps.hlsl";

auto shaders = manager.compileVariants(base, {
    {},                          // Base
    {"SKINNED"},                 // Skinned meshes
    {"INSTANCED"},               // GPU instancing
    {"SKINNED", "INSTANCED"},    // Both
    {"ALPHA_TEST"},              // Alpha testing
});
// Returns 5 std::shared_ptr<ShaderProgram>
```

### Hot-Reload

```cpp
desc.enableHotReload = true;

manager.setReloadCallback([](const std::string& key) {
    std::cout << "Shader reloaded: " << key << "\n";
    // Recreate GPU resources here...
});

auto shader = manager.compile(desc);

// Game loop
while (running) {
    manager.update(); // polls file-system for changes
    render();
}
```

### All Pipeline Stages

```cpp
desc.vsPath  = "mesh.vs.hlsl";
desc.gsPath  = "mesh.gs.hlsl";
desc.tcsPath = "mesh.hs.hlsl"; // Hull shader
desc.tesPath = "mesh.ds.hlsl"; // Domain shader
desc.psPath  = "mesh.ps.hlsl";

auto shader = manager.compile(desc);
```

### Compute Shader

```cpp
ShaderProgramCPU desc;
desc.csPath  = "particles.cs.hlsl";
desc.csEntry = "CSMain";
desc.defines = {{"THREAD_GROUP_SIZE", "64"}};

auto shader = manager.compile(desc);
auto& csBytecode = shader->stageBytecode[ShaderStage::Compute];
```

### Statistics

```cpp
const auto& stats = manager.getStatistics();
std::cout << "Compilations : " << stats.compilationCount   << "\n";
std::cout << "Cache hits   : " << stats.cacheHits          << "\n";
std::cout << "Average time : " << stats.averageCompileTime << " ms\n";
```

### Persistent Disk Cache

```cpp
// Pass a directory path; the manager will read/write .shcache files.
ShaderManager manager(GraphicsAPI::Vulkan, "/tmp/shader_cache");
```

## Graphics API Integration

### DirectX 11

```cpp
auto& bc = shader->stageBytecode[ShaderStage::Vertex];
ID3D11VertexShader* vs = nullptr;
device->CreateVertexShader(bc.data(), bc.size(), nullptr, &vs);
```

### DirectX 12

```cpp
auto& bc = shader->stageBytecode[ShaderStage::Vertex];
D3D12_SHADER_BYTECODE bytecode{ bc.data(), bc.size() };
psoDesc.VS = bytecode;
```

### Vulkan

```cpp
auto& bc = shader->stageBytecode[ShaderStage::Vertex];
VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
info.codeSize = bc.size();
info.pCode    = reinterpret_cast<const uint32_t*>(bc.data());
vkCreateShaderModule(device, &info, nullptr, &module);
```

## Production Integration

The backend compilers in `ShaderCompiler.cpp` ship with **mock implementations** that produce placeholder bytecode. To switch to real compilation:

| Target | Action |
|---|---|
| **DirectX 11** | Link `D3DCompiler.lib`, call `D3DCompile()` |
| **DirectX 12** | Link `dxcompiler.lib`, call DXC APIs |
| **OpenGL / Vulkan** | Integrate `glslang` or `shaderc` |
| **Metal** | Call `MTLDevice newLibraryWithSource:` |

Each integration point is marked with a `// --- PRODUCTION INTEGRATION POINT ---` comment in `ShaderCompiler.cpp`.

## Requirements

- C++17 or later
- CMake 3.16+
- No third-party dependencies for the core library (mock compilers only)

## License

See the root repository LICENSE file.
