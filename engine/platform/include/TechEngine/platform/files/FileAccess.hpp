#pragma once

#include <TechEngine/platform/files/FileResult.hpp>
#include <TechEngine/platform/files/MountTable.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngine {
    struct FileStatus {
        std::filesystem::path physicalPath;
        bool isDirectory = false;
        std::uint64_t size = 0;
        std::uint64_t lastModified = 0; // Seconds since the Unix epoch, on every platform.
    };

    class FileAccess {
    private:
        const MountTable* m_mounts = nullptr;

    public:
        explicit FileAccess(const MountTable& mounts);

        FileResult read(std::string_view virtualPath, std::vector<std::byte>& out) const;

        FileResult status(std::string_view virtualPath, FileStatus& out) const;

        FileResult list(std::string_view virtualPath, bool recursive, std::vector<std::string>& out) const;

        FileResult resolve(std::string_view virtualPath, std::filesystem::path& out) const;
    };
}
