#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/AssetIndex.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    struct IndexedAssetKind {
        BlobKind blob;
        StringId typeTag;
    };

    struct IndexedSide {
        Side side = Side::Common;
        std::string_view folder;
    };

    static constexpr std::array<IndexedAssetKind, 1> INDEXED_ASSET_KINDS{{
        {SCENE_KIND, SCENE_TYPE_TAG},
    }};

    static constexpr std::array<IndexedSide, 3> INDEXED_SIDES{{
        {Side::Common, "common"},
        {Side::Client, "client"},
        {Side::Server, "server"},
    }};

    static constexpr std::uint64_t ASSET_PREFIX_SIZE = BLOB_HEADER_SIZE + 8;

    std::error_code AssetIndex::build(const FileAccess& files, const std::string_view assetsRoot) {
        m_records.clear();
        m_duplicates.clear();

        std::vector<AssetRecord> found;
        for (const IndexedSide& side: INDEXED_SIDES) {
            std::vector<std::string> paths;
            const std::error_code listError = files.list(std::string(assetsRoot) + "/" + std::string(side.folder), true, paths);
            if (listError) {
                return listError;
            }

            for (const std::string& path: paths) {
                AssetRecord record;
                if (readRecord(files, path, side.side, record)) {
                    found.push_back(std::move(record));
                }
            }
        }

        std::ranges::sort(found, [](const AssetRecord& a, const AssetRecord& b) {
            if (a.id != b.id) {
                return a.id < b.id;
            }
            return a.path < b.path;
        });

        std::vector<AssetRecord> records;
        std::vector<DuplicateAsset> duplicates;
        records.reserve(found.size());
        std::size_t runStart = 0;
        while (runStart < found.size()) {
            std::size_t runEnd = runStart + 1;
            while (runEnd < found.size() && found[runEnd].id == found[runStart].id) {
                runEnd++;
            }

            if (runEnd - runStart == 1) {
                records.push_back(std::move(found[runStart]));
            } else {
                const AssetRecord& first = found[runStart];
                for (std::size_t i = runStart + 1; i < runEnd; i++) {
                    TE_LOGGER_WARN("Asset index: ID {0:#018x} is in both {1} and {2}, so neither is indexed", first.id.value(), first.path, found[i].path);
                    duplicates.push_back({first.id, first.path, found[i].path});
                }
            }
            runStart = runEnd;
        }

        m_records = std::move(records);
        m_duplicates = std::move(duplicates);
        return {};
    }

    bool AssetIndex::find(const ResourceId id, AssetRecord& out) const {
        const auto found = std::ranges::lower_bound(m_records, id, {}, &AssetRecord::id);
        if (found == m_records.end() || found->id != id) {
            return false;
        }
        out = *found;
        return true;
    }

    const std::vector<AssetRecord>& AssetIndex::getRecords() const {
        return m_records;
    }

    const std::vector<DuplicateAsset>& AssetIndex::getDuplicates() const {
        return m_duplicates;
    }

    bool AssetIndex::readRecord(const FileAccess& files, const std::string& path, const Side side, AssetRecord& out) const {
        FileStatus status;
        const std::error_code statusError = files.status(path, status);
        if (statusError) {
            TE_LOGGER_WARN("Asset index: skipped {0}: {1}", path, statusError.message());
            return false;
        }
        if (status.isDirectory) {
            return false;
        }
        if (status.size < ASSET_PREFIX_SIZE) {
            TE_LOGGER_WARN("Asset index: skipped {0}: {1} bytes is shorter than an asset header and ID", path, status.size);
            return false;
        }

        std::vector<std::byte> prefix;
        const std::error_code readError = files.readRange(path, 0, ASSET_PREFIX_SIZE, prefix);
        if (readError) {
            TE_LOGGER_WARN("Asset index: skipped {0}: {1}", path, readError.message());
            return false;
        }

        Reader magicReader{prefix};
        std::uint32_t magic = 0;
        magicReader.read(magic);
        const auto kind = std::ranges::find(INDEXED_ASSET_KINDS, magic, [](const IndexedAssetKind& candidate) {
            return candidate.blob.magic;
        });
        if (kind == INDEXED_ASSET_KINDS.end()) {
            TE_LOGGER_WARN("Asset index: skipped {0}: magic {1:#010x} is not an asset kind", path, magic);
            return false;
        }

        Reader reader{prefix};
        BlobHeader header;
        reader.readHeader(kind->blob, header);
        std::uint64_t id = 0;
        reader.read(id);
        if (!reader.ok()) {
            TE_LOGGER_WARN("Asset index: skipped {0}: {1}", path, reader.error().message());
            return false;
        }
        if (id == 0) {
            TE_LOGGER_WARN("Asset index: skipped {0}: its ID is null", path);
            return false;
        }

        out.id = ResourceId(id);
        out.typeTag = kind->typeTag;
        out.side = side;
        out.path = path;
        return true;
    }
}
