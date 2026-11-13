#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::uint32_t BC_BLOCK_SIZE = 4;
    static constexpr std::uint64_t BC_BLOCK_BYTES = 16;
    static constexpr std::uint64_t RGB9E5_PIXEL_BYTES = 4;

    std::uint32_t Texture::mipCount(const std::uint32_t width, const std::uint32_t height) {
        if (width == 0 || height == 0) {
            return 0;
        }
        return static_cast<std::uint32_t>(std::bit_width(std::max(width, height)));
    }

    std::uint64_t Texture::mipByteSize(const TextureFormat format, const std::uint32_t width, const std::uint32_t height, const std::uint32_t level) {
        const std::uint64_t levelWidth = std::max<std::uint64_t>(width >> level, 1);
        const std::uint64_t levelHeight = std::max<std::uint64_t>(height >> level, 1);

        std::uint64_t units = levelWidth * levelHeight;
        std::uint64_t unitBytes = RGB9E5_PIXEL_BYTES;
        if (format != TextureFormat::Rgb9E5) {
            units = ((levelWidth + BC_BLOCK_SIZE - 1) / BC_BLOCK_SIZE) * ((levelHeight + BC_BLOCK_SIZE - 1) / BC_BLOCK_SIZE);
            unitBytes = BC_BLOCK_BYTES;
        }
        if (units > std::numeric_limits<std::uint64_t>::max() / unitBytes) {
            return std::numeric_limits<std::uint64_t>::max();
        }
        return units * unitBytes;
    }

    void Texture::write(std::vector<std::byte>& out) const {
        TE_PROFILER_FUNCTION();
        Writer writer{out};
        writer.writeHeader(TEXTURE_KIND);
        writer.write(id.value());
        importBlock.write(writer);
        writer.write(static_cast<std::uint8_t>(shape));
        writer.write(static_cast<std::uint8_t>(format));
        writer.write(width);
        writer.write(height);
        const bool mipCountFits = TE_ENSURE(mips.size() <= BLOB_MAX_COUNT, "Texture of {0} mips exceeds the u32 count prefix; writing no mips", mips.size());
        writer.write(mipCountFits ? static_cast<std::uint32_t>(mips.size()) : static_cast<std::uint32_t>(0));
        writer.write(static_cast<std::uint8_t>(sampler.wrapU));
        writer.write(static_cast<std::uint8_t>(sampler.wrapV));
        writer.write(static_cast<std::uint8_t>(sampler.filter));
        if (!mipCountFits) {
            return;
        }
        for (const std::vector<std::byte>& mip: mips) {
            writer.writeSpan(std::span<const std::byte>{mip});
        }
    }

    std::error_code Texture::read(const std::span<const std::byte> bytes) {
        TE_PROFILER_FUNCTION();
        Reader reader{bytes};
        BlobHeader header;
        reader.readHeader(TEXTURE_KIND, header);
        std::uint64_t idValue = 0;
        reader.read(idValue);
        if (!reader.ok()) {
            return reader.error();
        }

        ImportBlock readImportBlock;
        const std::error_code importError = readImportBlock.read(reader);
        if (importError) {
            return importError;
        }

        std::uint8_t shapeValue = 0;
        std::uint8_t formatValue = 0;
        std::uint32_t readWidth = 0;
        std::uint32_t readHeight = 0;
        std::uint32_t readMipCount = 0;
        std::uint8_t wrapUValue = 0;
        std::uint8_t wrapVValue = 0;
        std::uint8_t filterValue = 0;
        reader.read(shapeValue);
        reader.read(formatValue);
        reader.read(readWidth);
        reader.read(readHeight);
        reader.read(readMipCount);
        reader.read(wrapUValue);
        reader.read(wrapVValue);
        reader.read(filterValue);
        if (!reader.ok()) {
            return reader.error();
        }

        const bool knownValues = shapeValue == static_cast<std::uint8_t>(TextureShape::Texture2D) && formatValue <= static_cast<std::uint8_t>(TextureFormat::Rgb9E5) && wrapUValue <= static_cast<std::uint8_t>(TextureWrap::Mirror) &&
                                 wrapVValue <= static_cast<std::uint8_t>(TextureWrap::Mirror) && filterValue <= static_cast<std::uint8_t>(TextureFilter::Nearest);
        if (!knownValues || readWidth == 0 || readHeight == 0 || readMipCount != mipCount(readWidth, readHeight)) {
            return ReadError::Malformed;
        }

        const TextureFormat readFormat = static_cast<TextureFormat>(formatValue);
        std::vector<std::vector<std::byte>> readMips(readMipCount);
        for (std::uint32_t level = 0; level < readMipCount; level++) {
            std::uint32_t byteCount = 0;
            reader.read(byteCount);
            if (!reader.ok()) {
                return reader.error();
            }
            if (byteCount != mipByteSize(readFormat, readWidth, readHeight, level)) {
                return ReadError::Malformed;
            }
            if (byteCount > reader.remaining()) {
                return ReadError::Truncated;
            }
            readMips[level].resize(byteCount);
            reader.readBytes(readMips[level]);
        }
        if (!reader.ok()) {
            return reader.error();
        }
        if (reader.remaining() != 0) {
            return ReadError::Malformed;
        }

        id = ResourceId{idValue};
        importBlock = std::move(readImportBlock);
        shape = static_cast<TextureShape>(shapeValue);
        format = readFormat;
        width = readWidth;
        height = readHeight;
        sampler = TextureSampler{static_cast<TextureWrap>(wrapUValue), static_cast<TextureWrap>(wrapVValue), static_cast<TextureFilter>(filterValue)};
        mips = std::move(readMips);
        return {};
    }

    std::error_code Texture::loadPayload(const std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies) {
        Texture* texture = static_cast<Texture*>(payload);
        dependencies.clear();
        return texture->read(bytes);
    }
}
