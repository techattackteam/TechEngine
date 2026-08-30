#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <chrono>
#include <fstream>
#include <ios>
#include <system_error>

namespace TechEngine {
    template<typename Iterator>
    static FileResult collectEntries(const std::filesystem::path& root, const std::string& base, std::vector<std::string>& out) {
        std::error_code ec;
        Iterator it{root, ec};
        if (ec) {
            return FileResult::IoError;
        }

        const Iterator end;
        while (it != end) {
            // lexically_relative, not relative(): the latter goes through weakly_canonical and
            // would resolve symlinks out of the path we are rebuilding.
            out.push_back(base + it->path().lexically_relative(root).generic_string());

            it.increment(ec);
            if (ec) {
                return FileResult::IoError;
            }
        }
        return FileResult::Ok;
    }

    FileAccess::FileAccess(const MountTable& mounts) : m_mounts{&mounts} {
    }

    FileResult FileAccess::read(std::string_view virtualPath, std::vector<std::byte>& out) const {
        std::filesystem::path physicalPath;
        const FileResult resolved = m_mounts->resolveExisting(virtualPath, physicalPath);
        if (resolved != FileResult::Ok) {
            return resolved;
        }

        std::error_code ec;
        if (std::filesystem::is_directory(physicalPath, ec)) {
            return FileResult::IsADirectory;
        }

        std::ifstream file{physicalPath, std::ios::binary | std::ios::ate};
        if (!file) {
            return FileResult::IoError;
        }

        const std::streamoff size = file.tellg();
        if (size < 0) {
            return FileResult::IoError;
        }

        out.resize(static_cast<std::size_t>(size));
        if (size == 0) {
            return FileResult::Ok;
        }

        file.seekg(0, std::ios::beg);
        if (!file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size))) {
            out.clear();
            return FileResult::IoError;
        }
        return FileResult::Ok;
    }

    FileResult FileAccess::write(std::string_view virtualPath, std::span<const std::byte> bytes) {
        std::filesystem::path physicalPath;
        const FileResult resolved = m_mounts->resolveForCreate(virtualPath, physicalPath);
        if (resolved != FileResult::Ok) {
            return resolved;
        }

        std::error_code ec;
        if (std::filesystem::is_directory(physicalPath, ec)) {
            return FileResult::IsADirectory;
        }

        std::ofstream file{physicalPath, std::ios::binary | std::ios::trunc};
        if (!file) {
            return FileResult::IoError;
        }

        if (!bytes.empty() && !file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
            return FileResult::IoError;
        }

        file.close();
        return file ? FileResult::Ok : FileResult::IoError;
    }

    FileResult FileAccess::status(std::string_view virtualPath, FileStatus& out) const {
        std::filesystem::path physicalPath;
        const FileResult resolved = m_mounts->resolveExisting(virtualPath, physicalPath);
        if (resolved != FileResult::Ok) {
            return resolved;
        }

        std::error_code ec;
        const bool isDirectory = std::filesystem::is_directory(physicalPath, ec);
        if (ec) {
            return FileResult::IoError;
        }

        std::uint64_t size = 0;
        if (!isDirectory) {
            size = std::filesystem::file_size(physicalPath, ec);
            if (ec) {
                return FileResult::IoError;
            }
        }

        const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(physicalPath, ec);
        if (ec) {
            return FileResult::IoError;
        }

        // file_time_type's epoch is unspecified — MSVC counts from 1601, libstdc++ from 1970.
        // An implementation may provide file_clock::to_sys or ::to_utc and need not provide
        // both, so clock_cast is the only portable spelling.
        const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(writeTime);

        out.physicalPath = physicalPath;
        out.isDirectory = isDirectory;
        out.size = size;
        out.lastModified = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(systemTime.time_since_epoch()).count());
        return FileResult::Ok;
    }

    FileResult FileAccess::list(std::string_view virtualPath, bool recursive, std::vector<std::string>& out) const {
        VirtualPathParts parts;
        if (!splitVirtualPath(virtualPath, parts)) {
            return FileResult::InvalidPath;
        }

        std::filesystem::path physicalPath;
        const FileResult resolved = m_mounts->resolveExisting(virtualPath, physicalPath);
        if (resolved != FileResult::Ok) {
            return resolved;
        }

        std::error_code ec;
        const bool isDirectory = std::filesystem::is_directory(physicalPath, ec);
        if (ec) {
            return FileResult::IoError;
        }
        if (!isDirectory) {
            return FileResult::NotADirectory;
        }

        std::string base = std::string{parts.alias} + "://";
        if (!parts.relative.empty()) {
            base += parts.relative;
            base += '/';
        }

        out.clear();
        const FileResult collected = recursive ? collectEntries<std::filesystem::recursive_directory_iterator>(physicalPath, base, out) : collectEntries<std::filesystem::directory_iterator>(physicalPath, base, out);
        if (collected != FileResult::Ok) {
            out.clear();
        }
        return collected;
    }

    FileResult FileAccess::resolve(std::string_view virtualPath, std::filesystem::path& out) const {
        return m_mounts->resolveExisting(virtualPath, out);
    }
}
