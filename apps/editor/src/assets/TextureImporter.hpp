#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ImageDecoder.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;

    enum class TexturePurpose : std::uint8_t {
        BaseColor,
        OcclusionRoughnessMetallic,
        Normal,
        Emissive,
    };

    struct SkyImportRequest {
        std::string sourcePath;
    };

    struct SkyImportResult {
        // The texture alone, or nothing for a re-import.
        std::vector<AssetRecord> records;
        // A re-import's texture, which keeps its id. Empty for an import.
        std::vector<ResourceId> rewritten;
    };

    class TextureImporter {
    private:
        FileAccess* m_files = nullptr;
        std::function<ResourceId()> m_generateId;

    public:
        // The largest value each RGB9E5 channel keeps: (511 / 512) * 2^16.
        static constexpr float RGB9E5_MAX_VALUE = 65408.0f;

        // Only importSky and reimportSky need an instance. generateId must return a new id on every call.
        TextureImporter(FileAccess& files, std::function<ResourceId()> generateId);

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

        // Every level after the source down to 1x1, largest first. The source is not copied into the list, so the chain
        // has Texture::mipCount - 1 entries. A source with a side of 0 gives none.
        static void buildMipChain(const FloatImage& source, std::vector<FloatImage>& out);

        // As buildMipChain, for unit normals in xyz: each level's xyz is normalized again before the next level is made
        // from it. A texel whose xyz averages to zero becomes (0, 0, 1). Alpha is filtered as in buildMipChain.
        static void buildNormalMapMipChain(const FloatImage& source, std::vector<FloatImage>& out);

        // Each side is halved, and a side of 1 stays 1. An output pixel is the mean of the 2x2 pixels at twice its
        // position, and a tap past the edge repeats the last pixel, so an odd side loses its last row or column.
        static void downsample(const FloatImage& above, FloatImage& out);

        // Negative and NaN channels become 0, and a channel above RGB9E5_MAX_VALUE becomes RGB9E5_MAX_VALUE. The result is
        // red in bits 0-8, green in 9-17, blue in 18-26 and the biased exponent in 27-31, as GL_RGB9_E5 stores it.
        static std::uint32_t packRgb9E5(float red, float green, float blue);

        static void unpackRgb9E5(std::uint32_t packed, float& red, float& green, float& blue);

        static std::string skyOutputPath(std::string_view sourcePath);

        // A Radiance .hdr becomes an RGB9E5 texture beside it. Refuses an output that exists. Writes nothing when it fails.
        // This and reimportSky wait while another import or re-import of the same source runs.
        std::error_code importSky(const SkyImportRequest& request, SkyImportResult& out) const;

        // Rewrites the texture an earlier importSky wrote, under its id. Writes nothing when it fails.
        std::error_code reimportSky(const SkyImportRequest& request, SkyImportResult& out) const;

        static bool isHdr(std::string_view sourcePath);

        // relativePath is a virtual path's part after the alias, such as assets/client/Sky.hdr.
        static bool sideOf(std::string_view relativePath, Side& out);

    private:
        std::error_code importSkyInto(const SkyImportRequest& request, bool reimport, SkyImportResult& out) const;

        static void toFloat(const ByteImage& image, TexturePurpose purpose, FloatImage& out);

        static void toBytes(const FloatImage& image, TexturePurpose purpose, ByteImage& out);

        static void scaleBilinear(const ByteImage& image, std::uint32_t width, std::uint32_t height, ByteImage& out);

        static void encodeLevel(const ByteImage& level, TexturePurpose purpose, std::vector<std::byte>& out);

        static void renormalize(FloatImage& image);

        static float clampRgb9E5Channel(float value);

        static void packRgb9E5Level(const FloatImage& image, std::vector<std::byte>& out);

        static float srgbToLinear(float value);

        static float linearToSrgb(float value);

        static std::uint8_t toByte(float value);

        static std::string_view wrapName(TextureWrap wrap);

        static char toLowerAscii(char character);
    };
}
