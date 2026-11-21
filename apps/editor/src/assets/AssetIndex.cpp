#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/AssetIndex.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    struct IndexedAssetKind {
        BlobKind blob;
        StringId typeTag;
        std::string_view extension;
    };

    struct IndexedSide {
        Side side = Side::Common;
        std::string_view folder;
    };

    struct SourceExtension {
        std::string_view extension;
        ListedKind kind = ListedKind::Other;
    };

    static constexpr std::array<IndexedAssetKind, 5> INDEXED_ASSET_KINDS{{
        {SCENE_KIND, SCENE_TYPE_TAG, ".tescene"},
        {MESH_KIND, MESH_TYPE_TAG, ".temesh"},
        {MODEL_KIND, MODEL_TYPE_TAG, ".temodel"},
        {TEXTURE_KIND, TEXTURE_TYPE_TAG, ".tetexture"},
        {MATERIAL_KIND, MATERIAL_TYPE_TAG, ".tematerial"},
    }};

    static constexpr std::array<SourceExtension, 5> SOURCE_EXTENSIONS{{
        {".gltf", ListedKind::ModelSource},
        {".glb", ListedKind::ModelSource},
        {".fbx", ListedKind::ModelSource},
        {".obj", ListedKind::ModelSource},
        {".hdr", ListedKind::SkySource},
    }};

    static constexpr std::array<IndexedSide, 3> INDEXED_SIDES{{
        {Side::Common, "common"},
        {Side::Client, "client"},
        {Side::Server, "server"},
    }};

    static constexpr std::uint64_t ASSET_PREFIX_SIZE = BLOB_HEADER_SIZE + 8;

    ResourceId randomResourceId() {
        std::random_device device;
        ResourceId id;
        do {
            const std::uint64_t high = device();
            const std::uint64_t low = device();
            id = ResourceId((high << 32) | low);
        } while (!id.valid());
        return id;
    }

    std::error_code AssetIndex::build(const FileAccess& files, const std::string_view assetsRoot) {
        std::vector<AssetRecord> found;
        std::vector<ListedEntry> listing;
        for (const IndexedSide& side: INDEXED_SIDES) {
            std::vector<std::string> paths;
            const std::error_code listError = files.list(std::string(assetsRoot) + "/" + std::string(side.folder), true, paths);
            if (listError) {
                return listError;
            }

            for (const std::string& path: paths) {
                FileStatus status;
                const std::error_code statusError = files.status(path, status);
                if (statusError) {
                    TE_LOGGER_WARN("Asset index: skipped {0}: {1}", path, statusError.message());
                    continue;
                }

                ListedEntry& entry = listing.emplace_back();
                entry.path = path;
                entry.side = side.side;
                if (status.isDirectory) {
                    entry.kind = ListedKind::Folder;
                    continue;
                }
                entry.kind = sourceKindOf(path);
                if (entry.kind != ListedKind::Other) {
                    continue;
                }

                AssetRecord record;
                if (readRecord(files, path, side.side, status.size, record)) {
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

        std::ranges::sort(listing, {}, &ListedEntry::path);
        for (const AssetRecord& record: records) {
            const auto entry = std::ranges::lower_bound(listing, record.path, {}, &ListedEntry::path);
            TE_CHECK(entry != listing.end() && entry->path == record.path, "Asset index: {0} was read but not listed", record.path);
            entry->kind = ListedKind::Asset;
            entry->id = record.id;
            entry->typeTag = record.typeTag;
        }

        m_assetsRoot = std::string(assetsRoot);
        m_records = std::move(records);
        m_duplicates = std::move(duplicates);
        m_listing = std::move(listing);
        m_listingRevision++;
        return {};
    }

    std::error_code AssetIndex::rebuild(const FileAccess& files) {
        TE_CHECK(!m_assetsRoot.empty(), "Asset index: rebuild before any build succeeded");
        return build(files, m_assetsRoot);
    }

    bool AssetIndex::find(const ResourceId id, AssetRecord& out) const {
        const auto found = std::ranges::lower_bound(m_records, id, {}, &AssetRecord::id);
        if (found == m_records.end() || found->id != id) {
            return false;
        }
        out = *found;
        return true;
    }

    ResourceId AssetIndex::generateId() {
        ResourceId id;
        do {
            id = randomResourceId();
        } while (isTaken(id));
        return id;
    }

    void AssetIndex::add(AssetRecord record) {
        TE_CHECK(record.id.valid(), "Asset index: {0} has a null ID", record.path);
        TE_CHECK(!isTaken(record.id), "Asset index: ID {0:#018x} of {1} is already in the index", record.id.value(), record.path);
        const auto position = std::ranges::lower_bound(m_records, record.id, {}, &AssetRecord::id);
        m_records.insert(position, std::move(record));
    }

    const std::vector<AssetRecord>& AssetIndex::getRecords() const {
        return m_records;
    }

    const std::vector<DuplicateAsset>& AssetIndex::getDuplicates() const {
        return m_duplicates;
    }

    const std::vector<ListedEntry>& AssetIndex::getListing() const {
        return m_listing;
    }

    std::uint64_t AssetIndex::getListingRevision() const {
        return m_listingRevision;
    }

    bool AssetIndex::readRecord(const FileAccess& files, const std::string& path, const Side side, const std::uint64_t size, AssetRecord& out) const {
        const bool engineFile = hasAssetExtension(path);
        if (size < ASSET_PREFIX_SIZE) {
            if (engineFile) {
                TE_LOGGER_WARN("Asset index: skipped {0}: {1} bytes is shorter than an asset header and ID", path, size);
            }
            return false;
        }

        std::vector<std::byte> prefix;
        const std::error_code readError = files.readRange(path, 0, ASSET_PREFIX_SIZE, prefix);
        if (readError) {
            if (engineFile) {
                TE_LOGGER_WARN("Asset index: skipped {0}: {1}", path, readError.message());
            }
            return false;
        }

        Reader magicReader{prefix};
        std::uint32_t magic = 0;
        magicReader.read(magic);
        const auto kind = std::ranges::find(INDEXED_ASSET_KINDS, magic, [](const IndexedAssetKind& candidate) {
            return candidate.blob.magic;
        });
        if (kind == INDEXED_ASSET_KINDS.end()) {
            if (engineFile) {
                TE_LOGGER_WARN("Asset index: skipped {0}: magic {1:#010x} is not an asset kind", path, magic);
            }
            return false;
        }

        Reader reader{prefix};
        BlobHeader header;
        reader.readHeader(kind->blob, header);
        std::uint64_t id = 0;
        reader.read(id);
        if (!reader.ok()) {
            if (engineFile) {
                TE_LOGGER_WARN("Asset index: skipped {0}: {1}", path, reader.error().message());
            }
            return false;
        }
        if (id == 0) {
            if (engineFile) {
                TE_LOGGER_WARN("Asset index: skipped {0}: its ID is null", path);
            }
            return false;
        }

        out.id = ResourceId(id);
        out.typeTag = kind->typeTag;
        out.side = side;
        out.path = path;
        return true;
    }

    bool AssetIndex::isTaken(const ResourceId id) const {
        const auto record = std::ranges::lower_bound(m_records, id, {}, &AssetRecord::id);
        if (record != m_records.end() && record->id == id) {
            return true;
        }
        return std::ranges::find(m_duplicates, id, &DuplicateAsset::id) != m_duplicates.end();
    }

    bool AssetIndex::hasAssetExtension(const std::string_view path) {
        const std::string extension = lowerExtensionOf(path);
        return std::ranges::find(INDEXED_ASSET_KINDS, extension, &IndexedAssetKind::extension) != INDEXED_ASSET_KINDS.end();
    }

    ListedKind AssetIndex::sourceKindOf(const std::string_view path) {
        const std::string extension = lowerExtensionOf(path);
        for (const auto& source: SOURCE_EXTENSIONS) {
            if (source.extension == extension) {
                return source.kind;
            }
        }
        return ListedKind::Other;
    }

    std::string AssetIndex::lowerExtensionOf(const std::string_view path) {
        const std::size_t slash = path.find_last_of('/');
        const std::size_t dot = path.find_last_of('.');
        if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
            return {};
        }
        std::string extension(path.substr(dot));
        std::ranges::transform(extension, extension.begin(), [](const char character) {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        });
        return extension;
    }
}
