#include <render/device/texture/Texture.hpp>

namespace TechEngine {
    std::size_t getReadBytesPerPixel(const GpuTextureFormat format) {
        switch (format) {
            case GpuTextureFormat::Rgba8:
                return 4;
            case GpuTextureFormat::Rgba16F:
                return 16;
            case GpuTextureFormat::Depth32F:
                return 4;
            case GpuTextureFormat::Bc7Srgb:
            case GpuTextureFormat::Bc7Unorm:
            case GpuTextureFormat::Bc5Unorm:
            case GpuTextureFormat::Rgb9E5:
                return 0;
        }
        return 0;
    }
}
