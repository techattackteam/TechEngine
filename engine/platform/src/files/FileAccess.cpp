#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <chrono>
#include <fstream>
#include <ios>
#include <system_error>

namespace TechEngine {
    template<typename Iterator>
    static std::error_code collectEntries(const std::filesystem::path& root, const std::string& base, std::vector<std::string>& out) {
        std::error_code ec;
        Iterator it{root, ec};
        if (ec) {
            return FileError::IoError;
        }

        const Iterator end;
        while (it != end) {
            // lexically_relative, not relative(): the latter goes through weakly_canonical and
            // would resolve symlinks out of the path we are rebuilding.
            out.push_back(base + it->path().lexically_relative(root).generic_string());

            it.increment(ec);
            if (ec) {
                return FileError::IoError;
            }
        }
        return {};
    }

    FileAccess::FileAccess(const MountTable& mounts) : m_mounts(&mounts) {
    }

    std::error_code FileAccess::read(std::string_view virtualPath, std::vector<std::byte>& out) const {
        std::filesystem::path physicalPath;
        const std::error_code resolveError = m_mounts->resolveExisting(virtualPath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        std::error_code ec;
        if (std::filesystem::is_directory(physicalPath, ec)) {
            return FileError::IsADirectory;
        }

        std::ifstream file{physicalPath, std::ios::binary | std::ios::ate};
        if (!file) {
            return FileError::IoError;
        }

        const std::streamoff size = file.tellg();
        if (size < 0) {
            return FileError::IoError;
        }

        out.resize(static_cast<std::size_t>(size));
        if (size == 0) {
            return {};
        }

        file.seekg(0, std::ios::beg);
        if (!file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size))) {
            out.clear();
            return FileError::IoError;
        }
        return {};
    }

    std::error_code FileAccess::write(std::string_view virtualPath, std::span<const std::byte> bytes) {
        std::filesystem::path physicalPath;
        const std::error_code resolveError = m_mounts->resolveForCreate(virtualPath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        std::error_code ec;
        if (std::filesystem::is_directory(physicalPath, ec)) {
            return FileError::IsADirectory;
        }

        std::ofstream file{physicalPath, std::ios::binary | std::ios::trunc};
        if (!file) {
            return FileError::NotFound;
        }

        if (!bytes.empty() && !file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
            return FileError::IoError;
        }

        file.close();
        if (!file) {
            return FileError::IoError;
        }
        return {};
    }

    std::error_code FileAccess::status(std::string_view virtualPath, FileStatus& out) const {
        std::filesystem::path physicalPath;
        const std::error_code resolveError = m_mounts->resolveExisting(virtualPath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        std::error_code ec;
        const bool isDirectory = std::filesystem::is_directory(physicalPath, ec);
        if (ec) {
            return FileError::IoError;
        }

        std::uint64_t size = 0;
        if (!isDirectory) {
            size = std::filesystem::file_size(physicalPath, ec);
            if (ec) {
                return FileError::IoError;
            }
        }

        const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(physicalPath, ec);
        if (ec) {
            return FileError::IoError;
        }

        // file_time_type's epoch is unspecified: MSVC counts from 1601, libstdc++ from 1970.
        // An implementation may provide file_clock::to_sys or ::to_utc and need not provide
        // both, so clock_cast is the only portable spelling.
        const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(writeTime);

        out.physicalPath = physicalPath;
        out.isDirectory = isDirectory;
        out.size = size;
        out.lastModified = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(systemTime.time_since_epoch()).count());
        return {};
    }

    std::error_code FileAccess::list(std::string_view virtualPath, bool recursive, std::vector<std::string>& out) const {
        VirtualPathParts parts;
        const std::error_code splitError = splitVirtualPath(virtualPath, parts);
        if (splitError) {
            return splitError;
        }

        std::filesystem::path physicalPath;
        const std::error_code resolveError = m_mounts->resolveExisting(virtualPath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        std::error_code ec;
        const bool isDirectory = std::filesystem::is_directory(physicalPath, ec);
        if (ec) {
            return FileError::IoError;
        }
        if (!isDirectory) {
            return FileError::NotADirectory;
        }

        std::string base = std::string{parts.alias} + "://";
        if (!parts.relative.empty()) {
            base += parts.relative;
            base += '/';
        }

        out.clear();
        const std::error_code collectError = recursive ? collectEntries<std::filesystem::recursive_directory_iterator>(physicalPath, base, out) : collectEntries<std::filesystem::directory_iterator>(physicalPath, base, out);
        if (collectError) {
            out.clear();
        }
        return collectError;
    }

    std::error_code FileAccess::resolve(const std::string_view virtualPath, std::filesystem::path& out) const {
        return m_mounts->resolveExisting(virtualPath, out);
    }

    std::error_code FileAccess::createDirectory(std::string_view virtualPath) {
        std::filesystem::path physicalPath;
        const std::error_code resolveError = m_mounts->resolveForCreate(virtualPath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        std::error_code ec;
        if (std::filesystem::exists(physicalPath, ec)) {
            return FileError::AlreadyExists;
        }

        std::filesystem::create_directories(physicalPath, ec);
        if (ec) {
            return FileError::IoError;
        }
        return {};
    }

    std::error_code FileAccess::remove(const std::string_view virtualPath, const bool recursive) {
        std::filesystem::path physicalPath;
        const std::error_code resolveError = m_mounts->resolveForCreate(virtualPath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        std::error_code ec;
        if (!std::filesystem::exists(physicalPath, ec)) {
            return FileError::NotFound;
        }

        if (!recursive && std::filesystem::is_directory(physicalPath, ec) && !std::filesystem::is_empty(physicalPath, ec)) {
            return FileError::NotEmpty;
        }

        if (recursive) {
            std::filesystem::remove_all(physicalPath, ec);
        } else {
            std::filesystem::remove(physicalPath, ec);
        }
        if (ec) {
            return FileError::IoError;
        }
        return {};
    }

    std::error_code FileAccess::copy(const std::string_view from, const std::string_view to) const {
        std::filesystem::path sourcePath;
        std::filesystem::path destinationPath;
        const std::error_code transferError = resolveTransfer(from, to, sourcePath, destinationPath);
        if (transferError) {
            return transferError;
        }

        std::error_code ec;
        if (std::filesystem::is_directory(sourcePath, ec)) {
            std::filesystem::copy(sourcePath, destinationPath, std::filesystem::copy_options::recursive, ec);
        } else {
            std::filesystem::copy_file(sourcePath, destinationPath, ec);
        }
        if (ec) {
            return FileError::IoError;
        }
        return {};
    }

    std::error_code FileAccess::move(const std::string_view from, const std::string_view to) const {
        std::filesystem::path sourcePath;
        std::filesystem::path destinationPath;
        const std::error_code transferError = resolveTransfer(from, to, sourcePath, destinationPath);
        if (transferError) {
            return transferError;
        }

        std::error_code ec;
        std::filesystem::rename(sourcePath, destinationPath, ec);
        if (ec) {
            return FileError::IoError;
        }
        return {};
    }

    std::error_code FileAccess::rename(const std::string_view virtualPath, const std::string_view newName) const {
        if (newName.empty() || newName == "." || newName == ".." || newName.find('/') != std::string_view::npos || newName.find('\\') != std::string_view::npos) {
            return FileError::InvalidPath;
        }

        std::filesystem::path physicalPath;
        const std::error_code resolveError = m_mounts->resolveExisting(virtualPath, physicalPath);
        if (resolveError) {
            return resolveError;
        }

        const std::filesystem::path newPath = physicalPath.parent_path() / newName;

        std::error_code ec;
        if (std::filesystem::exists(newPath, ec)) {
            return FileError::AlreadyExists;
        }

        std::filesystem::rename(physicalPath, newPath, ec);
        if (ec) {
            return FileError::IoError;
        }
        return {};
    }

    std::error_code FileAccess::resolveTransfer(const std::string_view from, const std::string_view to, std::filesystem::path& sourcePath, std::filesystem::path& destinationPath) const {
        const std::error_code sourceError = m_mounts->resolveExisting(from, sourcePath);
        if (sourceError) {
            return sourceError;
        }

        const std::error_code destinationError = m_mounts->resolveForCreate(to, destinationPath);
        if (destinationError) {
            return destinationError;
        }

        std::error_code ec;
        if (std::filesystem::exists(destinationPath, ec)) {
            return FileError::AlreadyExists;
        }
        if (!std::filesystem::exists(destinationPath.parent_path(), ec)) {
            return FileError::NotFound;
        }
        return {};
    }
}
