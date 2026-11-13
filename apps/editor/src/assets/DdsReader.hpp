#pragma once

#include <assets/ImageDecoder.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>

namespace TechEngine {
    class DdsReader {
    private:
        enum class Layout : std::uint8_t {
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
        static bool isDds(std::span<const std::byte> bytes);

        // Decodes the largest level of the first image, with no colour conversion: BC1, BC2, BC3, BC5 and BC7 from a
        // legacy or a DX10 header, and 32-bit RGBA or BGRA. BC5 holds red and green only, so blue is rebuilt as a unit
        // normal's z and alpha is 255. Any other format is UnsupportedDdsFormat, and a malformed file ImageUnreadable.
        static std::error_code decode(std::span<const std::byte> bytes, ByteImage& out);

    private:
        static std::error_code readLayout(std::span<const std::byte> bytes, Layout& layout, std::size_t& dataOffset);

        static bool layoutFromDxgi(std::uint32_t dxgiFormat, Layout& out);

        static bool layoutFromMasks(std::span<const std::byte> bytes, Layout& out);

        static std::uint64_t topLevelSize(Layout layout, std::uint32_t width, std::uint32_t height);

        static void decodeBlocks(std::span<const std::byte> data, Layout layout, ByteImage& out);

        static void decodePixels(std::span<const std::byte> data, Layout layout, ByteImage& out);

        static std::uint8_t rebuildNormalZ(std::uint8_t x, std::uint8_t y);

        static std::uint32_t readU32(std::span<const std::byte> bytes, std::size_t offset);
    };
}
