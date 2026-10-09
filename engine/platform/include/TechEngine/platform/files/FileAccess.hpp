#pragma once

#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
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

        std::error_code read(std::string_view virtualPath, std::vector<std::byte>& out) const;

        std::error_code write(std::string_view virtualPath, std::span<const std::byte> bytes);

        std::error_code status(std::string_view virtualPath, FileStatus& out) const;

        std::error_code list(std::string_view virtualPath, bool recursive, std::vector<std::string>& out) const;

        std::error_code resolve(std::string_view virtualPath, std::filesystem::path& out) const;

        std::error_code createDirectory(std::string_view virtualPath);

        std::error_code remove(std::string_view virtualPath, bool recursive);

        std::error_code copy(std::string_view from, std::string_view to) const;

        std::error_code move(std::string_view from, std::string_view to) const;

        std::error_code rename(std::string_view virtualPath, std::string_view newName) const;

    private:
        std::error_code resolveTransfer(std::string_view from, std::string_view to, std::filesystem::path& sourcePath, std::filesystem::path& destinationPath) const;
    };
}
