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
    private:
        enum class DdsLayout : std::uint8_t {
            Bc1,
            Bc2,
            Bc3,
            Bc5,
            Bc7,
            Rgba8,
            Bgra8,
            Rgbx8,
            Bgrx8,
        };

    public:
        // The longest side any decoder in the editor accepts.
        static constexpr std::uint32_t MAX_IMAGE_SIDE = 16384;

        // Reads PNG, JPEG, TGA and Radiance HDR. An 8-bit file is converted to linear with stb_image's 2.2 gamma, and a
        // file with no alpha gets 1. Every decode call refuses a side above MAX_IMAGE_SIDE before decoding.
        static std::error_code decodeFloat(std::span<const std::byte> bytes, FloatImage& out);

        // A DDS file goes through decodeDds, and PNG, JPEG, TGA and Radiance HDR through stb_image. A Radiance HDR file is
        // tone-mapped to 8 bits by stb_image, so use decodeFloat for one.
        static std::error_code decodeBytes(std::span<const std::byte> bytes, ByteImage& out);

        static bool isDds(std::span<const std::byte> bytes);

        // Decodes the largest level of the first image, with no colour conversion: BC1, BC2, BC3, BC5 and BC7 from a
        // legacy or a DX10 header, and 32-bit RGBA or BGRA. BC5 holds red and green only, so blue is rebuilt as a unit
        // normal's z and alpha is 255. Any other format is UnsupportedDdsFormat, and a malformed file ImageUnreadable.
        static std::error_code decodeDds(std::span<const std::byte> bytes, ByteImage& out);

    private:
        static bool isDecodable(std::span<const std::byte> bytes);

        static std::error_code readDdsLayout(std::span<const std::byte> bytes, DdsLayout& layout, std::size_t& dataOffset);

        static bool layoutFromDxgi(std::uint32_t dxgiFormat, DdsLayout& out);

        static bool layoutFromMasks(std::span<const std::byte> bytes, DdsLayout& out);

        static std::uint64_t topLevelSize(DdsLayout layout, std::uint32_t width, std::uint32_t height);

        static void decodeBlocks(std::span<const std::byte> data, DdsLayout layout, ByteImage& out);

        static void decodePixels(std::span<const std::byte> data, DdsLayout layout, ByteImage& out);

        static std::uint8_t rebuildNormalZ(std::uint8_t x, std::uint8_t y);

        static std::uint32_t readU32(std::span<const std::byte> bytes, std::size_t offset);
    };
}
