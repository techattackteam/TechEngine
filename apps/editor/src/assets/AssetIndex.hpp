#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;

    struct AssetRecord {
        ResourceId id;
        StringId typeTag;
        Side side = Side::Common;
        std::string path;
    };

    struct DuplicateAsset {
        ResourceId id;
        std::string firstPath;
        std::string secondPath;
    };

    enum class ListedKind : std::uint8_t {
        Folder,
        Asset,
        ModelSource,
        SkySource,
        Other,
    };

    struct ListedEntry {
        std::string path;
        Side side = Side::Common;
        ListedKind kind = ListedKind::Other;
        // Set only on an Asset entry.
        ResourceId id;
        StringId typeTag;
    };

    struct SideFolder {
        Side side = Side::Common;
        std::string path;
    };

    // Safe from any thread. Nothing checks the id against an index, so the caller does.
    ResourceId randomResourceId();

    class AssetIndex {
    private:
        std::string m_assetsRoot;
        std::vector<AssetRecord> m_records;
        std::vector<DuplicateAsset> m_duplicates;
        std::vector<ListedEntry> m_listing;
        std::uint64_t m_listingRevision = 0;

    public:
        // A failed build keeps the records and the listing of the last build that succeeded.
        std::error_code build(const FileAccess& files, std::string_view assetsRoot);

        // Builds again from the root of the last successful build.
        std::error_code rebuild(const FileAccess& files);

        bool find(ResourceId id, AssetRecord& out) const;

        ResourceId generateId();

        void add(AssetRecord record);

        const std::vector<AssetRecord>& getRecords() const;

        const std::vector<DuplicateAsset>& getDuplicates() const;

        // Sorted by path.
        const std::vector<ListedEntry>& getListing() const;

        std::uint64_t getListingRevision() const;

        // The listing leaves the side folders out. Empty until a build succeeds.
        void collectSideFolders(std::vector<SideFolder>& out) const;

    private:
        bool readRecord(const FileAccess& files, const std::string& path, Side side, std::uint64_t size, AssetRecord& out) const;

        bool isTaken(ResourceId id) const;

        static bool hasAssetExtension(std::string_view path);

        static ListedKind sourceKindOf(std::string_view path);

        // Empty when the file name has no dot.
        static std::string lowerExtensionOf(std::string_view path);
    };
}
