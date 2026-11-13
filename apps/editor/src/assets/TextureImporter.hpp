#pragma once

#include <TechEngine/core/texture/Texture.hpp>

#include <assets/ImageDecoder.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngine {
    enum class TexturePurpose : std::uint8_t {
        BaseColor,
        OcclusionRoughnessMetallic,
        Normal,
        Emissive,
    };

    class TextureImporter {
    public:
        static TextureFormat formatOf(TexturePurpose purpose);

        static std::string_view purposeName(TexturePurpose purpose);

        // "repeat-linear", or with two wraps "repeat-clamp-linear", U first.
        static std::string samplerName(const TextureSampler& sampler);

        // "<image>#<purpose>#<sampler>", the name in a texture's source key.
        static std::string keyName(std::string_view imageName, TexturePurpose purpose, const TextureSampler& sampler);

        // Red from occlusion, green and blue from metallicRoughness, and alpha 255. Both are scaled bilinearly to the
        // larger width and the larger height, and an image with a side of 0 gives its channels 255.
        static void packOcclusionRoughnessMetallic(const ByteImage& occlusion, const ByteImage& metallicRoughness, ByteImage& out);

        // Fills the format, the size and the full mip chain from 8-bit RGBA, and leaves the rest of out alone. Base color
        // and emissive are sRGB and filtered in linear light; a normal map's xyz is renormalized at every level.
        static void encode(const ByteImage& image, TexturePurpose purpose, Texture& out);

    private:
        static void toFloat(const ByteImage& image, TexturePurpose purpose, FloatImage& out);

        static void toBytes(const FloatImage& image, TexturePurpose purpose, ByteImage& out);

        static void scaleBilinear(const ByteImage& image, std::uint32_t width, std::uint32_t height, ByteImage& out);

        static void encodeLevel(const ByteImage& level, TexturePurpose purpose, std::vector<std::byte>& out);

        static float srgbToLinear(float value);

        static float linearToSrgb(float value);

        static std::uint8_t toByte(float value);

        static std::string_view wrapName(TextureWrap wrap);
    };
}
