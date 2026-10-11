#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class FileAccess;

    struct PackEntry {
        ResourceId id;
        StringId typeTag;
        std::uint64_t offset = 0;
        std::uint64_t size = 0;
    };

    class PackReader {
    private:
        const FileAccess* m_files = nullptr;
        std::string m_path;
        std::vector<PackEntry> m_entries;
        PackEntry m_bootRecord;

    public:
        explicit PackReader(const FileAccess& files);

        std::error_code open(std::string_view virtualPath);

        bool find(ResourceId id, PackEntry& out) const;

        bool bootRecord(PackEntry& out) const;

        const std::vector<PackEntry>& getEntries() const;

        std::error_code read(const PackEntry& entry, std::vector<std::byte>& out) const;
    };
}
