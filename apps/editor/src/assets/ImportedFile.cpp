#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/ImportedFile.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    struct ImportedFileKind {
        BlobKind blob;
        StringId typeTag;
    };

    static constexpr std::array<ImportedFileKind, 4> IMPORTED_FILE_KINDS{{
        {MODEL_KIND, MODEL_TYPE_TAG},
        {MESH_KIND, MESH_TYPE_TAG},
        {MATERIAL_KIND, MATERIAL_TYPE_TAG},
        {TEXTURE_KIND, TEXTURE_TYPE_TAG},
    }};

    // The id follows the header, and the import block follows the id with its own byte count first.
    static constexpr std::uint64_t IMPORT_BLOCK_OFFSET = BLOB_HEADER_SIZE + sizeof(std::uint64_t);
    static constexpr std::uint64_t IMPORT_BLOCK_COUNT_SIZE = sizeof(std::uint32_t);

    std::error_code readImportedFile(const FileAccess& files, const std::string_view path, ImportedFile& out) {
        std::vector<std::byte> prefix;
        const std::error_code prefixError = files.readRange(path, 0, IMPORT_BLOCK_OFFSET + IMPORT_BLOCK_COUNT_SIZE, prefix);
        if (prefixError) {
            return prefixError;
        }

        Reader prefixReader{prefix};
        std::uint32_t magic = 0;
        prefixReader.read(magic);
        const auto kind = std::ranges::find(IMPORTED_FILE_KINDS, magic, [](const ImportedFileKind& candidate) {
            return candidate.blob.magic;
        });
        if (kind == IMPORTED_FILE_KINDS.end()) {
            return ReadError::BadMagic;
        }
        std::uint32_t blockSize = 0;
        Reader countReader{std::span<const std::byte>(prefix).subspan(IMPORT_BLOCK_OFFSET)};
        countReader.read(blockSize);

        std::vector<std::byte> bytes;
        const std::error_code readError = files.readRange(path, 0, IMPORT_BLOCK_OFFSET + IMPORT_BLOCK_COUNT_SIZE + blockSize, bytes);
        if (readError) {
            return readError;
        }
        Reader reader{bytes};
        BlobHeader header;
        reader.readHeader(kind->blob, header);
        std::uint64_t id = 0;
        reader.read(id);
        if (!reader.ok()) {
            return reader.error();
        }
        ImportBlock importBlock;
        const std::error_code blockError = importBlock.read(reader);
        if (blockError) {
            return blockError;
        }

        out.path = std::string(path);
        out.typeTag = kind->typeTag;
        out.id = ResourceId(id);
        out.importBlock = std::move(importBlock);
        return {};
    }

    std::error_code listImportedFiles(const FileAccess& files, const std::string_view folder, std::vector<ImportedFile>& out) {
        std::vector<std::string> paths;
        const std::error_code listError = files.list(folder, true, paths);
        if (listError) {
            return listError;
        }
        std::ranges::sort(paths);

        out.clear();
        for (const std::string& path: paths) {
            FileStatus status;
            const std::error_code statusError = files.status(path, status);
            if (statusError) {
                TE_LOGGER_WARN("Re-import: skipped {0}: {1}", path, statusError.message());
                continue;
            }
            if (status.isDirectory) {
                continue;
            }
            ImportedFile file;
            const std::error_code readError = readImportedFile(files, path, file);
            if (readError) {
                TE_LOGGER_WARN("Re-import: skipped {0}, which is not an imported file: {1}", path, readError.message());
                continue;
            }
            out.push_back(std::move(file));
        }
        return {};
    }
}
