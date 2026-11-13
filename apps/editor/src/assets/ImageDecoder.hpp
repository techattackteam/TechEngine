#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    struct FloatImage {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        // Four floats a pixel, row by row from the top, in linear light.
        std::vector<float> pixels;
    };

    struct ByteImage {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        // Four bytes a pixel, row by row from the top, with no colour conversion.
        std::vector<std::uint8_t> pixels;
    };

    class ImageDecoder {
    public:
        // Reads PNG, JPEG, TGA and Radiance HDR. An 8-bit file is converted to linear with stb_image's 2.2 gamma, and a
        // file with no alpha gets 1.
        static std::error_code decodeFloat(std::span<const std::byte> bytes, FloatImage& out);

        // A Radiance HDR file is tone-mapped to 8 bits by stb_image, so use decodeFloat for one.
        static std::error_code decodeBytes(std::span<const std::byte> bytes, ByteImage& out);
    };
}
