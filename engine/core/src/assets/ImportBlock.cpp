#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>

namespace TechEngine {
    void ImportBlock::write(Writer& writer) const {
        const std::uint32_t byteCount = static_cast<std::uint32_t>(sizeof(std::uint32_t) + sourcePath.size() + sizeof(std::uint32_t) + sizeof(std::uint32_t) + sourceName.size() + sizeof(float) + sizeof(std::uint8_t));
        writer.write(byteCount);

        writer.write(sourcePath);
        writer.write(sourceIndex);
        writer.write(sourceName);
        writer.write(settings.scale);
        writer.write(static_cast<std::uint8_t>(settings.upAxis));
    }

    std::error_code ImportBlock::read(Reader& reader) {
        std::uint32_t byteCount = 0;
        reader.read(byteCount);
        if (!reader.ok()) {
            return reader.error();
        }

        const std::size_t remainingBefore = reader.remaining();
        std::string readSourcePath;
        reader.read(readSourcePath);
        std::uint32_t readSourceIndex = 0;
        reader.read(readSourceIndex);
        std::string readSourceName;
        reader.read(readSourceName);
        float readScale = 0.0f;
        reader.read(readScale);
        std::uint8_t readUpAxis = 0;
        reader.read(readUpAxis);
        if (!reader.ok()) {
            return reader.error();
        }

        if (remainingBefore - reader.remaining() != byteCount) {
            return ReadError::Malformed;
        }
        if (readUpAxis > static_cast<std::uint8_t>(UpAxis::Z)) {
            return ReadError::Malformed;
        }

        sourcePath = std::move(readSourcePath);
        sourceIndex = readSourceIndex;
        sourceName = std::move(readSourceName);
        settings.scale = readScale;
        settings.upAxis = static_cast<UpAxis>(readUpAxis);
        return {};
    }

    void ImportBlock::writeAssetPrologue(Writer& writer, const BlobKind& kind, const ResourceId id) const {
        writer.writeHeader(kind);
        writer.write(id.value());
        write(writer);
    }

    std::error_code ImportBlock::readAssetPrologue(Reader& reader, const BlobKind& kind, ResourceId& id) {
        BlobHeader header;
        reader.readHeader(kind, header);
        std::uint64_t idValue = 0;
        reader.read(idValue);
        if (!reader.ok()) {
            return reader.error();
        }

        const std::error_code blockError = read(reader);
        if (blockError) {
            return blockError;
        }
        id = ResourceId{idValue};
        return {};
    }
}
