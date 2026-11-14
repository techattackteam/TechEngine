#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    inline constexpr BlobKind TEXTURE_KIND{0x58544554, 1};

    inline constexpr StringId TEXTURE_TYPE_TAG{"TechEngine.Texture"};

    enum class TextureShape : std::uint8_t {
        Texture2D = 0,
    };

    enum class TextureFormat : std::uint8_t {
        Bc7Srgb = 0,
        Bc7Unorm,
        Bc5Unorm,
        Rgb9E5,
    };

    enum class TextureWrap : std::uint8_t {
        Repeat = 0,
        Clamp,
        Mirror,
    };

    enum class TextureFilter : std::uint8_t {
        Linear = 0,
        Nearest,
    };

    struct TextureSampler {
        TextureWrap wrapU = TextureWrap::Repeat;
        TextureWrap wrapV = TextureWrap::Repeat;
        TextureFilter filter = TextureFilter::Linear;

        bool operator==(const TextureSampler&) const = default;
    };

    struct Texture {
        ResourceId id;
        ImportBlock importBlock;
        TextureShape shape = TextureShape::Texture2D;
        TextureFormat format = TextureFormat::Bc7Srgb;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        TextureSampler sampler;
        // Largest level first. A valid texture holds the full chain down to 1x1.
        std::vector<std::vector<std::byte>> mips;

        // The number of levels in the full chain for a size, or 0 when either side is 0.
        static std::uint32_t mipCount(std::uint32_t width, std::uint32_t height);

        // The bytes one level takes in its format, or UINT64_MAX when they overflow 64 bits.
        // BC data is in 4x4 blocks, so a level smaller than 4 pixels still takes one block.
        static std::uint64_t mipByteSize(TextureFormat format, std::uint32_t width, std::uint32_t height, std::uint32_t level);

        void write(std::vector<std::byte>& out) const;

        std::error_code read(std::span<const std::byte> bytes);

        static std::error_code loadPayload(std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies);

        bool operator==(const Texture&) const = default;
    };
}
