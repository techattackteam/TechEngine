#include <assets/MipBuilder.hpp>
#include <assets/TextureImporter.hpp>

#include <bc7enc.h>
#include <rgbcx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::size_t CHANNELS = 4;
    static constexpr std::uint32_t BLOCK_SIDE = 4;
    static constexpr std::size_t BLOCK_BYTES = 16;
    static constexpr std::uint8_t MISSING_CHANNEL = 255;
    // bc7enc's fastest search level, scanning a quarter of mode 1's partitions.
    static constexpr std::uint32_t BC7_UBER_LEVEL = 0;
    static constexpr std::uint32_t BC7_MAX_PARTITIONS = 16;

    // Both encoders fill global tables once, and neither may run while that happens.
    static std::once_flag g_encodersReady;

    TextureFormat TextureImporter::formatOf(const TexturePurpose purpose) {
        switch (purpose) {
            case TexturePurpose::BaseColor:
            case TexturePurpose::Emissive:
                return TextureFormat::Bc7Srgb;
            case TexturePurpose::OcclusionRoughnessMetallic:
                return TextureFormat::Bc7Unorm;
            case TexturePurpose::Normal:
                return TextureFormat::Bc5Unorm;
        }
        return TextureFormat::Bc7Unorm;
    }

    std::string_view TextureImporter::purposeName(const TexturePurpose purpose) {
        switch (purpose) {
            case TexturePurpose::BaseColor:
                return "basecolor";
            case TexturePurpose::OcclusionRoughnessMetallic:
                return "orm";
            case TexturePurpose::Normal:
                return "normal";
            case TexturePurpose::Emissive:
                return "emissive";
        }
        return "unknown";
    }

    std::string TextureImporter::samplerName(const TextureSampler& sampler) {
        std::string name(wrapName(sampler.wrapU));
        if (sampler.wrapV != sampler.wrapU) {
            name += "-";
            name += wrapName(sampler.wrapV);
        }
        name += sampler.filter == TextureFilter::Nearest ? "-nearest" : "-linear";
        return name;
    }

    std::string TextureImporter::keyName(const std::string_view imageName, const TexturePurpose purpose, const TextureSampler& sampler) {
        return std::string(imageName) + "#" + std::string(purposeName(purpose)) + "#" + samplerName(sampler);
    }

    void TextureImporter::packOcclusionRoughnessMetallic(const ByteImage& occlusion, const ByteImage& metallicRoughness, ByteImage& out) {
        const bool hasOcclusion = occlusion.width != 0 && occlusion.height != 0;
        const bool hasMetallicRoughness = metallicRoughness.width != 0 && metallicRoughness.height != 0;
        out.width = std::max(hasOcclusion ? occlusion.width : 1, hasMetallicRoughness ? metallicRoughness.width : 1);
        out.height = std::max(hasOcclusion ? occlusion.height : 1, hasMetallicRoughness ? metallicRoughness.height : 1);

        ByteImage scaledOcclusion;
        if (hasOcclusion) {
            scaleBilinear(occlusion, out.width, out.height, scaledOcclusion);
        }
        ByteImage scaledMetallicRoughness;
        if (hasMetallicRoughness) {
            scaleBilinear(metallicRoughness, out.width, out.height, scaledMetallicRoughness);
        }

        const std::size_t pixelCount = static_cast<std::size_t>(out.width) * out.height;
        out.pixels.assign(pixelCount * CHANNELS, MISSING_CHANNEL);
        for (std::size_t pixel = 0; pixel < pixelCount; pixel++) {
            std::uint8_t* destination = &out.pixels[pixel * CHANNELS];
            if (hasOcclusion) {
                destination[0] = scaledOcclusion.pixels[pixel * CHANNELS];
            }
            if (hasMetallicRoughness) {
                destination[1] = scaledMetallicRoughness.pixels[pixel * CHANNELS + 1];
                destination[2] = scaledMetallicRoughness.pixels[pixel * CHANNELS + 2];
            }
        }
    }

    // The source's own bytes are encoded as the top level, so only the levels below it go through float.
    void TextureImporter::encode(const ByteImage& image, const TexturePurpose purpose, Texture& out) {
        std::call_once(g_encodersReady, [] {
            rgbcx::init();
            bc7enc_compress_block_init();
        });

        out.format = formatOf(purpose);
        out.width = image.width;
        out.height = image.height;
        out.mips.clear();
        if (image.width == 0 || image.height == 0) {
            return;
        }

        FloatImage source;
        toFloat(image, purpose, source);
        std::vector<FloatImage> chain;
        if (purpose == TexturePurpose::Normal) {
            MipBuilder::buildNormalMap(source, chain);
        } else {
            MipBuilder::build(source, chain);
        }
        source.pixels.clear();

        out.mips.resize(chain.size() + 1);
        encodeLevel(image, purpose, out.mips[0]);
        ByteImage level;
        for (std::size_t i = 0; i < chain.size(); i++) {
            toBytes(chain[i], purpose, level);
            encodeLevel(level, purpose, out.mips[i + 1]);
        }
    }

    void TextureImporter::toFloat(const ByteImage& image, const TexturePurpose purpose, FloatImage& out) {
        std::array<float, 256> table{};
        for (std::size_t value = 0; value < table.size(); value++) {
            const float unit = static_cast<float>(value) / 255.0f;
            switch (purpose) {
                case TexturePurpose::BaseColor:
                case TexturePurpose::Emissive:
                    table[value] = srgbToLinear(unit);
                    break;
                case TexturePurpose::OcclusionRoughnessMetallic:
                    table[value] = unit;
                    break;
                case TexturePurpose::Normal:
                    table[value] = unit * 2.0f - 1.0f;
                    break;
            }
        }

        out.width = image.width;
        out.height = image.height;
        out.pixels.resize(image.pixels.size());
        for (std::size_t i = 0; i < image.pixels.size(); i++) {
            const bool alpha = i % CHANNELS == CHANNELS - 1;
            out.pixels[i] = alpha ? static_cast<float>(image.pixels[i]) / 255.0f : table[image.pixels[i]];
        }
    }

    void TextureImporter::toBytes(const FloatImage& image, const TexturePurpose purpose, ByteImage& out) {
        out.width = image.width;
        out.height = image.height;
        out.pixels.resize(image.pixels.size());
        for (std::size_t i = 0; i < image.pixels.size(); i++) {
            const float value = image.pixels[i];
            if (i % CHANNELS == CHANNELS - 1) {
                out.pixels[i] = toByte(value);
                continue;
            }
            switch (purpose) {
                case TexturePurpose::BaseColor:
                case TexturePurpose::Emissive:
                    out.pixels[i] = toByte(linearToSrgb(value));
                    break;
                case TexturePurpose::OcclusionRoughnessMetallic:
                    out.pixels[i] = toByte(value);
                    break;
                case TexturePurpose::Normal:
                    out.pixels[i] = toByte(value * 0.5f + 0.5f);
                    break;
            }
        }
    }

    // Texel centres line up, and a tap past the edge takes the edge texel.
    void TextureImporter::scaleBilinear(const ByteImage& image, const std::uint32_t width, const std::uint32_t height, ByteImage& out) {
        out.width = width;
        out.height = height;
        out.pixels.resize(static_cast<std::size_t>(width) * height * CHANNELS);
        const float scaleX = static_cast<float>(image.width) / static_cast<float>(width);
        const float scaleY = static_cast<float>(image.height) / static_cast<float>(height);
        for (std::uint32_t y = 0; y < height; y++) {
            const float sourceY = std::clamp((static_cast<float>(y) + 0.5f) * scaleY - 0.5f, 0.0f, static_cast<float>(image.height - 1));
            const std::uint32_t y0 = static_cast<std::uint32_t>(sourceY);
            const std::uint32_t y1 = std::min(y0 + 1, image.height - 1);
            const float weightY = sourceY - static_cast<float>(y0);
            for (std::uint32_t x = 0; x < width; x++) {
                const float sourceX = std::clamp((static_cast<float>(x) + 0.5f) * scaleX - 0.5f, 0.0f, static_cast<float>(image.width - 1));
                const std::uint32_t x0 = static_cast<std::uint32_t>(sourceX);
                const std::uint32_t x1 = std::min(x0 + 1, image.width - 1);
                const float weightX = sourceX - static_cast<float>(x0);
                for (std::size_t channel = 0; channel < CHANNELS; channel++) {
                    const auto texel = [&image, channel](const std::uint32_t texelX, const std::uint32_t texelY) {
                        return static_cast<float>(image.pixels[(static_cast<std::size_t>(texelY) * image.width + texelX) * CHANNELS + channel]);
                    };
                    const float top = texel(x0, y0) + (texel(x1, y0) - texel(x0, y0)) * weightX;
                    const float bottom = texel(x0, y1) + (texel(x1, y1) - texel(x0, y1)) * weightX;
                    out.pixels[(static_cast<std::size_t>(y) * width + x) * CHANNELS + channel] = toByte((top + (bottom - top) * weightY) / 255.0f);
                }
            }
        }
    }

    // A level smaller than a block repeats its last row and column to fill the block.
    void TextureImporter::encodeLevel(const ByteImage& level, const TexturePurpose purpose, std::vector<std::byte>& out) {
        bc7enc_compress_block_params bc7Params{};
        bc7enc_compress_block_params_init(&bc7Params);
        bc7Params.m_uber_level = BC7_UBER_LEVEL;
        bc7Params.m_max_partitions = BC7_MAX_PARTITIONS;
        if (purpose == TexturePurpose::OcclusionRoughnessMetallic) {
            bc7enc_compress_block_params_init_linear_weights(&bc7Params);
        }

        const std::uint32_t blocksAcross = (level.width + BLOCK_SIDE - 1) / BLOCK_SIDE;
        const std::uint32_t blocksDown = (level.height + BLOCK_SIDE - 1) / BLOCK_SIDE;
        out.resize(static_cast<std::size_t>(blocksAcross) * blocksDown * BLOCK_BYTES);
        std::array<std::uint8_t, BLOCK_SIDE * BLOCK_SIDE * CHANNELS> block{};
        for (std::uint32_t blockY = 0; blockY < blocksDown; blockY++) {
            for (std::uint32_t blockX = 0; blockX < blocksAcross; blockX++) {
                for (std::uint32_t y = 0; y < BLOCK_SIDE; y++) {
                    const std::uint32_t sourceY = std::min(blockY * BLOCK_SIDE + y, level.height - 1);
                    for (std::uint32_t x = 0; x < BLOCK_SIDE; x++) {
                        const std::uint32_t sourceX = std::min(blockX * BLOCK_SIDE + x, level.width - 1);
                        const std::uint8_t* texel = &level.pixels[(static_cast<std::size_t>(sourceY) * level.width + sourceX) * CHANNELS];
                        std::copy_n(texel, CHANNELS, &block[(y * BLOCK_SIDE + x) * CHANNELS]);
                    }
                }

                std::byte* destination = &out[(static_cast<std::size_t>(blockY) * blocksAcross + blockX) * BLOCK_BYTES];
                if (purpose == TexturePurpose::Normal) {
                    rgbcx::encode_bc5(destination, block.data());
                } else {
                    bc7enc_compress_block(destination, block.data(), &bc7Params);
                }
            }
        }
    }

    float TextureImporter::srgbToLinear(const float value) {
        if (value <= 0.04045f) {
            return value / 12.92f;
        }
        return std::pow((value + 0.055f) / 1.055f, 2.4f);
    }

    float TextureImporter::linearToSrgb(const float value) {
        if (value <= 0.0031308f) {
            return value * 12.92f;
        }
        return 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
    }

    std::uint8_t TextureImporter::toByte(const float value) {
        return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
    }

    std::string_view TextureImporter::wrapName(const TextureWrap wrap) {
        switch (wrap) {
            case TextureWrap::Repeat:
                return "repeat";
            case TextureWrap::Clamp:
                return "clamp";
            case TextureWrap::Mirror:
                return "mirror";
        }
        return "repeat";
    }
}
