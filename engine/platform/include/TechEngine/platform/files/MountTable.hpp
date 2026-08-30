#pragma once

#include <TechEngine/platform/files/FileResult.hpp>

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngine {
    struct MountEntry {
        std::string alias;
        std::filesystem::path physicalRoot;
        std::filesystem::path canonicalRoot; // Empty when physicalRoot did not exist at mount time
        int priority = 0;
    };

    class MountTable {
    private:
        std::vector<MountEntry> m_entries;

    public:
        void mount(const std::string& alias, const std::filesystem::path& physicalRoot, int priority = 0);

        bool unmount(const std::string& alias);

        void clear();

        bool hasAlias(std::string_view alias) const;

        std::size_t mountCount() const;

        std::span<const MountEntry> entries() const;

        FileResult resolveExisting(std::string_view virtualPath, std::filesystem::path& out) const;

        FileResult resolveForCreate(std::string_view virtualPath, std::filesystem::path& out) const;
    };
}
