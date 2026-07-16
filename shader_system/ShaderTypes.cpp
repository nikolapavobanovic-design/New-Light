#include "ShaderTypes.h"

// ShaderTypes.h is intentionally header-only for the simple POD types.
// This translation unit exists as a placeholder so that larger future
// additions (e.g. operator<< overloads, serialisation helpers) have a
// natural home without breaking existing include graphs.

namespace ShaderSystem {

// Returns a human-readable name for a ShaderStage value.
const char* stageName(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex:      return "Vertex";
        case ShaderStage::Pixel:       return "Pixel";
        case ShaderStage::Compute:     return "Compute";
        case ShaderStage::Geometry:    return "Geometry";
        case ShaderStage::TessControl: return "TessControl";
        case ShaderStage::TessEval:    return "TessEval";
        default:                       return "Unknown";
    }
}

// Returns a human-readable name for a GraphicsAPI value.
const char* apiName(GraphicsAPI api) {
    switch (api) {
        case GraphicsAPI::DirectX11: return "DirectX11";
        case GraphicsAPI::DirectX12: return "DirectX12";
        case GraphicsAPI::OpenGL:    return "OpenGL";
        case GraphicsAPI::Vulkan:    return "Vulkan";
        case GraphicsAPI::Metal:     return "Metal";
        default:                     return "Unknown";
    }
}

} // namespace ShaderSystem
