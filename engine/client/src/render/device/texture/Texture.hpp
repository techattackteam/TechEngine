#pragma once

#include <render/device/RenderExtent.hpp>

#include <cstddef>
#include <cstdint>

namespace TechEngine {
    enum class GpuTextureFormat : std::uint8_t { Rgba8, Rgba16F, Depth32F, Bc7Srgb, Bc7Unorm, Bc5Unorm, Rgb9E5 };

    enum class GpuTextureWrap : std::uint8_t { Repeat, Clamp, Mirror };

    enum class GpuTextureFilter : std::uint8_t { Linear, Nearest };

    struct TextureDescription {
        int width = 0;
        int height = 0;
        int mipLevels = 1;
        GpuTextureFormat format = GpuTextureFormat::Rgba8;
        GpuTextureWrap wrapU = GpuTextureWrap::Repeat;
        GpuTextureWrap wrapV = GpuTextureWrap::Repeat;
        GpuTextureFilter filter = GpuTextureFilter::Linear;
    };

    // Rgba16F reads back as 32-bit floats, not halves, so a caller gets values it can use directly.
    // A format readTexture cannot read returns 0.
    std::size_t getReadBytesPerPixel(GpuTextureFormat format);

    struct GpuTexture {
        unsigned int id = 0;
        RenderExtent extent;
        GpuTextureFormat format = GpuTextureFormat::Rgba8;
        int mipLevels = 1;
        std::uint64_t bindlessHandle = 0;
    };
}
