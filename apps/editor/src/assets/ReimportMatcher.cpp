#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/ReimportMatcher.hpp>

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

    // Exact keys are matched over every output before any name alone is, so an output whose index moved cannot take
    // the file of an unchanged output that shares its name.
    void ReimportMatcher::match(const std::span<const ImportKey> oldKeys, const std::span<const ImportKey> wanted, std::vector<std::uint32_t>& out) {
        out.assign(wanted.size(), REIMPORT_NO_MATCH);
        std::vector<bool> taken(oldKeys.size(), false);
        for (std::size_t i = 0; i < wanted.size(); i++) {
            for (std::size_t old = 0; old < oldKeys.size(); old++) {
                if (!taken[old] && sameTypeAndName(oldKeys[old], wanted[i]) && oldKeys[old].sourceIndex == wanted[i].sourceIndex) {
                    out[i] = static_cast<std::uint32_t>(old);
                    taken[old] = true;
                    break;
                }
            }
        }

        for (std::size_t i = 0; i < wanted.size(); i++) {
            if (out[i] != REIMPORT_NO_MATCH || wanted[i].sourceName.empty()) {
                continue;
            }
            std::uint32_t best = REIMPORT_NO_MATCH;
            for (std::size_t old = 0; old < oldKeys.size(); old++) {
                if (taken[old] || !sameTypeAndName(oldKeys[old], wanted[i])) {
                    continue;
                }
                if (best == REIMPORT_NO_MATCH || oldKeys[old].sourceIndex < oldKeys[best].sourceIndex) {
                    best = static_cast<std::uint32_t>(old);
                }
            }
            if (best != REIMPORT_NO_MATCH) {
                out[i] = best;
                taken[best] = true;
            }
        }
    }

    bool ReimportMatcher::sameTypeAndName(const ImportKey& first, const ImportKey& second) {
        return first.typeTag == second.typeTag && first.sourceName == second.sourceName;
    }

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
