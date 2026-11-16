#pragma once

#include <render/device/RenderExtent.hpp>

#include <cstddef>
#include <cstdint>

namespace TechEngine {
    enum class TextureFormat : std::uint8_t { Rgba8, Rgba16F, Depth32F };

    struct TextureDescription {
        int width = 0;
        int height = 0;
        int mipLevels = 1;
        TextureFormat format = TextureFormat::Rgba8;
    };

    // Rgba16F reads back as 32-bit floats, not halves, so a caller gets values it can use directly.
    inline constexpr std::size_t getReadBytesPerPixel(const TextureFormat format) {
        switch (format) {
            case TextureFormat::Rgba8:
                return 4;
            case TextureFormat::Rgba16F:
                return 16;
            case TextureFormat::Depth32F:
                return 4;
        }
        return 0;
    }

    struct GpuTexture {
        unsigned int id = 0;
        RenderExtent extent;
        TextureFormat format = TextureFormat::Rgba8;
    };
}
