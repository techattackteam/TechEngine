#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <assets/ReimportMatcher.hpp>
#include <assets/TextureImporter.hpp>
#include <project/ProjectError.hpp>

#include <bc7enc.h>
#include <rgbcx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view HDR_EXTENSION = ".hdr";
    static constexpr std::string_view TEXTURE_EXTENSION = ".tetexture";
    static constexpr std::string_view ASSETS_FOLDER = "assets/";
    static constexpr std::size_t CHANNELS = 4;
    static constexpr std::uint32_t BLOCK_SIDE = 4;
    static constexpr std::size_t BLOCK_BYTES = 16;
    static constexpr std::uint8_t MISSING_CHANNEL = 255;
    // bc7enc's fastest search level, scanning a quarter of mode 1's partitions.
    static constexpr std::uint32_t BC7_UBER_LEVEL = 0;
    static constexpr std::uint32_t BC7_MAX_PARTITIONS = 16;
    static constexpr float TAP_WEIGHT = 0.25f;
    static constexpr float MIN_NORMAL_LENGTH_SQUARED = 1.0e-12f;
    static constexpr int MANTISSA_BITS = 9;
    static constexpr int EXPONENT_BIAS = 15;
    static constexpr std::uint32_t MANTISSA_MASK = (1u << MANTISSA_BITS) - 1;
    static constexpr std::uint32_t MAX_MANTISSA = MANTISSA_MASK;
    static constexpr int EXPONENT_SHIFT = 27;
    static constexpr std::size_t PACKED_BYTES = 4;
    // A lat-long sky wraps around in longitude and ends at the poles.
    static constexpr TextureSampler SKY_SAMPLER{TextureWrap::Repeat, TextureWrap::Clamp, TextureFilter::Linear};

    // Both encoders fill global tables once, and neither may run while that happens.
    static std::once_flag g_encodersReady;

    TextureImporter::TextureImporter(FileAccess& files, std::function<ResourceId()> generateId) : m_files(&files), m_generateId(std::move(generateId)) {
    }

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
            buildNormalMapMipChain(source, chain);
        } else {
            buildMipChain(source, chain);
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

    void TextureImporter::buildMipChain(const FloatImage& source, std::vector<FloatImage>& out) {
        out.clear();
        if (source.width == 0 || source.height == 0) {
            return;
        }
        std::uint32_t width = source.width;
        std::uint32_t height = source.height;
        while (width > 1 || height > 1) {
            const FloatImage& above = out.empty() ? source : out.back();
            FloatImage level;
            downsample(above, level);
            width = level.width;
            height = level.height;
            out.push_back(std::move(level));
        }
    }

    void TextureImporter::buildNormalMapMipChain(const FloatImage& source, std::vector<FloatImage>& out) {
        out.clear();
        if (source.width == 0 || source.height == 0) {
            return;
        }
        std::uint32_t width = source.width;
        std::uint32_t height = source.height;
        while (width > 1 || height > 1) {
            const FloatImage& above = out.empty() ? source : out.back();
            FloatImage level;
            downsample(above, level);
            renormalize(level);
            width = level.width;
            height = level.height;
            out.push_back(std::move(level));
        }
    }

    void TextureImporter::downsample(const FloatImage& above, FloatImage& out) {
        out.width = std::max<std::uint32_t>(above.width / 2, 1);
        out.height = std::max<std::uint32_t>(above.height / 2, 1);
        out.pixels.assign(static_cast<std::size_t>(out.width) * out.height * CHANNELS, 0.0f);

        for (std::uint32_t y = 0; y < out.height; y++) {
            for (std::uint32_t x = 0; x < out.width; x++) {
                float* destination = &out.pixels[(static_cast<std::size_t>(y) * out.width + x) * CHANNELS];
                for (std::uint32_t tapY = 0; tapY < 2; tapY++) {
                    const std::uint32_t sourceY = std::min(y * 2 + tapY, above.height - 1);
                    for (std::uint32_t tapX = 0; tapX < 2; tapX++) {
                        const std::uint32_t sourceX = std::min(x * 2 + tapX, above.width - 1);
                        const float* tap = &above.pixels[(static_cast<std::size_t>(sourceY) * above.width + sourceX) * CHANNELS];
                        for (std::size_t channel = 0; channel < CHANNELS; channel++) {
                            destination[channel] += tap[channel] * TAP_WEIGHT;
                        }
                    }
                }
            }
        }
    }

    std::uint32_t TextureImporter::packRgb9E5(const float red, const float green, const float blue) {
        const float clampedRed = clampRgb9E5Channel(red);
        const float clampedGreen = clampRgb9E5Channel(green);
        const float clampedBlue = clampRgb9E5Channel(blue);
        const float largest = std::max({clampedRed, clampedGreen, clampedBlue});

        int exponent = std::max(-EXPONENT_BIAS - 1, std::ilogb(largest)) + 1 + EXPONENT_BIAS;
        float denominator = std::exp2(static_cast<float>(exponent - EXPONENT_BIAS - MANTISSA_BITS));
        if (static_cast<std::uint32_t>(std::floor(largest / denominator + 0.5f)) == MAX_MANTISSA + 1) {
            denominator *= 2.0f;
            exponent++;
        }

        const std::uint32_t redMantissa = static_cast<std::uint32_t>(std::floor(clampedRed / denominator + 0.5f));
        const std::uint32_t greenMantissa = static_cast<std::uint32_t>(std::floor(clampedGreen / denominator + 0.5f));
        const std::uint32_t blueMantissa = static_cast<std::uint32_t>(std::floor(clampedBlue / denominator + 0.5f));
        return (static_cast<std::uint32_t>(exponent) << EXPONENT_SHIFT) | (blueMantissa << (2 * MANTISSA_BITS)) | (greenMantissa << MANTISSA_BITS) | redMantissa;
    }

    void TextureImporter::unpackRgb9E5(const std::uint32_t packed, float& red, float& green, float& blue) {
        const int exponent = static_cast<int>(packed >> EXPONENT_SHIFT);
        const float scale = std::exp2(static_cast<float>(exponent - EXPONENT_BIAS - MANTISSA_BITS));
        red = static_cast<float>(packed & MANTISSA_MASK) * scale;
        green = static_cast<float>((packed >> MANTISSA_BITS) & MANTISSA_MASK) * scale;
        blue = static_cast<float>((packed >> (2 * MANTISSA_BITS)) & MANTISSA_MASK) * scale;
    }

    std::string TextureImporter::skyOutputPath(const std::string_view sourcePath) {
        const std::size_t slash = sourcePath.find_last_of('/');
        const std::size_t dot = sourcePath.find_last_of('.');
        const bool hasExtension = dot != std::string_view::npos && (slash == std::string_view::npos || dot > slash + 1);
        const std::string_view stem = hasExtension ? sourcePath.substr(0, dot) : sourcePath;
        return std::string(stem) + std::string(TEXTURE_EXTENSION);
    }

    std::error_code TextureImporter::importSky(const SkyImportRequest& request, SkyImportResult& out) const {
        return importSkyInto(request, false, out);
    }

    std::error_code TextureImporter::reimportSky(const SkyImportRequest& request, SkyImportResult& out) const {
        return importSkyInto(request, true, out);
    }

    bool TextureImporter::isHdr(const std::string_view sourcePath) {
        const std::size_t slash = sourcePath.find_last_of('/');
        const std::size_t dot = sourcePath.find_last_of('.');
        if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
            return false;
        }
        std::string extension(sourcePath.substr(dot));
        std::ranges::transform(extension, extension.begin(), toLowerAscii);
        return extension == HDR_EXTENSION;
    }

    bool TextureImporter::sideOf(const std::string_view relativePath, Side& out) {
        if (!relativePath.starts_with(ASSETS_FOLDER)) {
            return false;
        }
        const std::string_view inside = relativePath.substr(ASSETS_FOLDER.size());
        const std::size_t slash = inside.find('/');
        if (slash == std::string_view::npos) {
            return false;
        }
        static constexpr std::array<std::pair<std::string_view, Side>, 3> SIDES{{{"common", Side::Common}, {"client", Side::Client}, {"server", Side::Server}}};
        const std::string_view name = inside.substr(0, slash);
        for (const std::pair<std::string_view, Side>& side: SIDES) {
            if (side.first == name) {
                out = side.second;
                return true;
            }
        }
        return false;
    }

    std::error_code TextureImporter::importSkyInto(const SkyImportRequest& request, const bool reimport, SkyImportResult& out) const {
        TE_PROFILER_FUNCTION();
        if (!isHdr(request.sourcePath)) {
            return ProjectError::UnsupportedImageType;
        }

        VirtualPathParts parts;
        const std::error_code splitError = splitVirtualPath(request.sourcePath, parts);
        if (splitError) {
            return splitError;
        }
        Side side = Side::Common;
        if (!sideOf(parts.relative, side)) {
            return ProjectError::SourceNotInAssets;
        }

        const std::string path = skyOutputPath(request.sourcePath);
        FileStatus outputStatus;
        const std::error_code outputError = m_files->status(path, outputStatus);
        ResourceId keptId;
        if (!reimport) {
            if (!outputError) {
                return ProjectError::ImportOutputExists;
            }
            if (outputError != FileError::NotFound) {
                return outputError;
            }
        } else {
            if (outputError == FileError::NotFound) {
                return ProjectError::ReimportTargetMissing;
            }
            if (outputError) {
                return outputError;
            }
            ImportedFile existing;
            const std::error_code existingError = readImportedFile(*m_files, path, existing);
            if (existingError) {
                return existingError;
            }
            if (existing.typeTag != TEXTURE_TYPE_TAG) {
                return ProjectError::ReimportTargetMissing;
            }
            keptId = existing.id;
        }

        std::vector<std::byte> sourceBytes;
        const std::error_code readError = m_files->read(request.sourcePath, sourceBytes);
        if (readError) {
            return readError;
        }
        FloatImage image;
        const std::error_code decodeError = ImageDecoder::decodeFloat(sourceBytes, image);
        if (decodeError) {
            return decodeError;
        }

        Texture texture;
        texture.id = reimport ? keptId : m_generateId();
        texture.importBlock.sourcePath = std::string(parts.relative);
        texture.format = TextureFormat::Rgb9E5;
        texture.width = image.width;
        texture.height = image.height;
        texture.sampler = SKY_SAMPLER;

        std::vector<FloatImage> chain;
        buildMipChain(image, chain);
        texture.mips.resize(chain.size() + 1);
        packRgb9E5Level(image, texture.mips[0]);
        for (std::size_t level = 0; level < chain.size(); level++) {
            packRgb9E5Level(chain[level], texture.mips[level + 1]);
        }

        std::vector<std::byte> bytes;
        texture.write(bytes);
        const std::error_code writeError = reimport ? m_files->write(path, bytes) : m_files->createFile(path, bytes);
        if (writeError == FileError::AlreadyExists) {
            return ProjectError::ImportOutputExists;
        }
        if (writeError) {
            return writeError;
        }

        out.records.clear();
        out.rewritten.clear();
        if (reimport) {
            out.rewritten.push_back(texture.id);
        } else {
            out.records.push_back(AssetRecord{texture.id, TEXTURE_TYPE_TAG, side, path});
        }
        return {};
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

    void TextureImporter::renormalize(FloatImage& image) {
        for (std::size_t pixel = 0; pixel < image.pixels.size(); pixel += CHANNELS) {
            float* texel = &image.pixels[pixel];
            const float lengthSquared = texel[0] * texel[0] + texel[1] * texel[1] + texel[2] * texel[2];
            if (lengthSquared < MIN_NORMAL_LENGTH_SQUARED) {
                texel[0] = 0.0f;
                texel[1] = 0.0f;
                texel[2] = 1.0f;
                continue;
            }
            const float inverseLength = 1.0f / std::sqrt(lengthSquared);
            texel[0] *= inverseLength;
            texel[1] *= inverseLength;
            texel[2] *= inverseLength;
        }
    }

    float TextureImporter::clampRgb9E5Channel(const float value) {
        if (!(value > 0.0f)) {
            return 0.0f;
        }
        return std::min(value, RGB9E5_MAX_VALUE);
    }

    void TextureImporter::packRgb9E5Level(const FloatImage& image, std::vector<std::byte>& out) {
        const std::size_t pixelCount = static_cast<std::size_t>(image.width) * image.height;
        out.resize(pixelCount * PACKED_BYTES);
        for (std::size_t pixel = 0; pixel < pixelCount; pixel++) {
            const float* source = &image.pixels[pixel * CHANNELS];
            const std::uint32_t packed = packRgb9E5(source[0], source[1], source[2]);
            for (std::size_t byte = 0; byte < PACKED_BYTES; byte++) {
                out[pixel * PACKED_BYTES + byte] = static_cast<std::byte>((packed >> (8 * byte)) & 0xFF);
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

    char TextureImporter::toLowerAscii(const char character) {
        if (character >= 'A' && character <= 'Z') {
            return static_cast<char>(character - 'A' + 'a');
        }
        return character;
    }
}
