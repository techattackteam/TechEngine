#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>

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

    class AssetIndex {
    private:
        std::vector<AssetRecord> m_records;
        std::vector<DuplicateAsset> m_duplicates;

    public:
        std::error_code build(const FileAccess& files, std::string_view assetsRoot);

        bool find(ResourceId id, AssetRecord& out) const;

        ResourceId generateId();

        void add(AssetRecord record);

        const std::vector<AssetRecord>& getRecords() const;

        const std::vector<DuplicateAsset>& getDuplicates() const;

    private:
        bool readRecord(const FileAccess& files, const std::string& path, Side side, AssetRecord& out) const;

        bool isTaken(ResourceId id) const;
    };
}
