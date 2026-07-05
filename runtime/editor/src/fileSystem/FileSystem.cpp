#include "FileSystem.hpp"
#include "core/Logger.hpp"
#include <fstream>
#include <ranges>
#include <chrono>

namespace TechEngine {
    void FileSystem::mount(const std::filesystem::path& alias, const std::filesystem::path& physicalPath, int priority) {
        std::unique_lock lock(m_mutex);

        m_mountEntries.push_back({alias, physicalPath, priority});
        std::ranges::sort(m_mountEntries, [](const MountEntry& a, const MountEntry& b) {
            return a.priority > b.priority;
        });

        TE_LOGGER_INFO("[FileSystem] Mounted '{0}' → '{1}' (priority {2})", alias.string(), physicalPath.string(), priority);
    }

    void FileSystem::unmount(const std::filesystem::path& virtualPath) {
        std::unique_lock lock(m_mutex);

        auto it = std::ranges::remove_if(m_mountEntries, [&](const MountEntry& mp) {
            return mp.alias == virtualPath;
        }).begin();

        if (it != m_mountEntries.end()) {
            TE_LOGGER_INFO("[FileSystem] Unmounted '{0}'", virtualPath.string());
            m_mountEntries.erase(it, m_mountEntries.end());
        } else {
            TE_LOGGER_WARN("[FileSystem] No mount entry found for virtual path: '{0}'", virtualPath.string());
        }
    }


    bool FileSystem::read(const std::filesystem::path& virtualPath, Buffer& outBuffer) {
        std::shared_lock lock(m_mutex);
        std::filesystem::path physicalPath = resolveInternal(virtualPath.string());

        if (physicalPath.empty()) {
            TE_LOGGER_ERROR("[FileSystem] File not found: '{0}'", virtualPath.string());
            return false;
        }
        std::ifstream file(physicalPath, std::ios::binary | std::ios::ate);
        if (!file) {
            TE_LOGGER_ERROR("[FileSystem] Failed to open file for reading: '{0}' (resolved to '{1}')", virtualPath.string(), physicalPath.string());
            return false;
        }
        const size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        outBuffer.resize(fileSize);
        if (!file.read(reinterpret_cast<char*>(outBuffer.data), static_cast<std::streamsize>(fileSize))) {
            TE_LOGGER_ERROR("[FileSystem] Failed to read file: '{0}' (resolved to '{1}')", virtualPath.string(), physicalPath.string());
            return false;
        }

        file.close();
        return true;
    }

    bool FileSystem::write(const std::filesystem::path& virtualPath, const Buffer& buffer) {
        std::unique_lock lock(m_mutex);

        auto [alias, rel] = splitVirtual(virtualPath);
        std::filesystem::path physicalPath;
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == alias) {
                physicalPath = mount.physicalRoot / rel;
                break;
            }
        }
        if (physicalPath.empty()) {
            TE_LOGGER_ERROR("[FileSystem] No mount entry found for virtual path: '{0}'", virtualPath.string());
            return false;
        }
        std::error_code ec;
        std::filesystem::create_directories(physicalPath.parent_path(), ec);
        std::ofstream file(physicalPath, std::ios::binary | std::ios::trunc);
        if (!file) {
            TE_LOGGER_ERROR("[FileSystem] Failed to open file for writing: '{0}' (resolved to '{1}')", virtualPath.string(), physicalPath.string());
            return false;
        }

        file.write(reinterpret_cast<const char*>(buffer.data), static_cast<std::streamsize>(buffer.size));
        file.close();
        return true;
    }

    FileStatus FileSystem::status(const std::filesystem::path& virtualPath) {
        std::shared_lock lock(m_mutex);
        const std::filesystem::path physicalPath = resolveInternal(virtualPath);
        if (physicalPath.empty()) {
            TE_LOGGER_ERROR("[FileSystem] File not found: '{0}'", virtualPath.string());
            return {};
        }
        std::error_code ec;
        const std::filesystem::file_status status = std::filesystem::status(physicalPath, ec);

        auto [alias, _] = splitVirtual(virtualPath);
        FileStatus fileStatus;
        fileStatus.alias = alias;
        fileStatus.name = physicalPath.stem().string();
        fileStatus.extension = physicalPath.extension().string();
        fileStatus.virtualPath = virtualPath;
        if (!ec) {
            fileStatus.exists = std::filesystem::exists(status);
            fileStatus.isDirectory = std::filesystem::is_directory(status);
            fileStatus.size = fileStatus.exists && !fileStatus.isDirectory ? std::filesystem::file_size(physicalPath, ec) : 0;
            fileStatus.lastModified = fileStatus.exists ? std::chrono::duration_cast<std::chrono::nanoseconds>(std::filesystem::last_write_time(physicalPath, ec).time_since_epoch()).count() : 0;
            if (ec) {
                TE_LOGGER_ERROR("[FileSystem] Failed to get file last modified time: '{0}' (resolved to '{1}')", virtualPath.string(), physicalPath.string());
            }
        }
        return fileStatus;
    }

    std::filesystem::path FileSystem::resolve(const std::filesystem::path& virtualPath) {
        std::shared_lock lock(m_mutex);
        return resolveInternal(virtualPath);
    }

    std::vector<std::filesystem::path> FileSystem::list(const std::filesystem::path& virtualPath, bool recursive) {
        std::shared_lock lock(m_mutex);
        const std::filesystem::path physicalPath = resolveInternal(virtualPath);
        if (physicalPath.empty()) {
            TE_LOGGER_ERROR("[FileSystem] Directory not found: '{0}'", virtualPath.string());
            return {};
        }

        auto [alias, relative] = splitVirtual(virtualPath);

        std::vector<std::filesystem::path> result;
        if (recursive) {
            for (const auto& entry: std::filesystem::recursive_directory_iterator(physicalPath)) {
                if (!entry.is_regular_file()) {
                    continue;
                }
                std::filesystem::path virtPath = std::filesystem::relative(entry.path(), physicalPath);
                if (!relative.empty()) {
                    virtPath = relative / virtPath;
                }
                result.emplace_back(alias.string() + virtPath.string());
            }
        } else {
            for (const auto& entry: std::filesystem::directory_iterator(physicalPath)) {
                std::filesystem::path virtualChild = std::filesystem::relative(entry.path(), physicalPath);
                if (!relative.empty()) {
                    virtualChild = relative / virtualChild;
                }
                result.emplace_back(alias.string() + virtualChild.string());
            }
        }
        return result;
    }

    bool FileSystem::createDirectory(const std::filesystem::path& virtualPath) {
        std::unique_lock lock(m_mutex);
        auto [alias, rel] = splitVirtual(virtualPath);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == alias) {
                std::filesystem::path physicalPath = mount.physicalRoot / rel;
                std::error_code ec;
                std::filesystem::create_directories(physicalPath, ec);
                return !ec;
            }
        }
        return false;
    }

    bool FileSystem::deleteDirectory(const std::filesystem::path& virtualPath) {
        std::unique_lock lock(m_mutex);
        auto [alias, rel] = splitVirtual(virtualPath);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == alias) {
                std::filesystem::path physicalPath = mount.physicalRoot / rel;
                std::error_code ec;
                std::filesystem::remove_all(physicalPath, ec);
                return !ec;
            }
        }
        return false;
    }

    bool FileSystem::copyDirectory(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) {
        std::unique_lock lock(m_mutex);
        auto [sourceAlias, sourceRel] = splitVirtual(virtualSource);
        auto [destAlias, destRel] = splitVirtual(virtualDestination);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == sourceAlias && mount.alias == destAlias) {
                std::filesystem::path sourcePhysicalPath = mount.physicalRoot / sourceRel;
                std::filesystem::path destPhysicalPath = mount.physicalRoot / destRel;
                std::error_code ec;
                std::filesystem::copy(sourcePhysicalPath, destPhysicalPath, std::filesystem::copy_options::recursive, ec);
                return !ec;
            }
        }
        return false;
    }

    bool FileSystem::moveDirectory(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) {
        std::unique_lock lock(m_mutex);
        auto [sourceAlias, sourceRel] = splitVirtual(virtualSource);
        auto [destAlias, destRel] = splitVirtual(virtualDestination);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == sourceAlias && mount.alias == destAlias) {
                std::filesystem::path sourcePhysicalPath = mount.physicalRoot / sourceRel;
                std::filesystem::path destPhysicalPath = mount.physicalRoot / destRel;
                std::error_code ec;
                std::filesystem::rename(sourcePhysicalPath, destPhysicalPath, ec); //For some reason in the c++ lib rename can be used to move
                return !ec;
            }
        }
        return false;
    }

    bool FileSystem::renameDirectory(const std::filesystem::path& virtualSource, const std::string& virtualDestination) {
        std::unique_lock lock(m_mutex);
        auto [sourceAlias, sourceRel] = splitVirtual(virtualSource);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == sourceAlias) {
                std::filesystem::path sourcePhysicalPath = mount.physicalRoot / sourceRel;
                std::filesystem::path destPhysicalPath = sourcePhysicalPath.parent_path() / virtualDestination;
                std::error_code ec;
                std::filesystem::rename(sourcePhysicalPath, destPhysicalPath, ec);
                return !ec;
            }
        }
        return false;
    }

    std::filesystem::path FileSystem::getParentPath(const std::filesystem::path& virtualPath) {
        std::shared_lock lock(m_mutex);
        const std::filesystem::path physicalPath = resolveInternal(virtualPath);
        if (physicalPath.empty()) {
            TE_LOGGER_ERROR("[FileSystem] File not found: '{0}'", virtualPath.string());
            return {};
        }
        std::filesystem::path parentPhysicalPath = physicalPath.parent_path();
        for (const MountEntry& mount: m_mountEntries) {
            if (parentPhysicalPath == mount.physicalRoot || parentPhysicalPath.string().find(mount.physicalRoot.string() + "/") == 0) {
                std::filesystem::path relativeParent = std::filesystem::relative(parentPhysicalPath, mount.physicalRoot);
                if (relativeParent == ".") {
                    return mount.alias;
                } else {
                    return mount.alias / relativeParent;
                }
            }
        }
        return {};
    }

    bool FileSystem::isDirectory(const std::filesystem::path& virtualPath) {
        std::shared_lock lock(m_mutex);
        const std::filesystem::path physicalPath = resolveInternal(virtualPath);
        if (physicalPath.empty()) {
            TE_LOGGER_ERROR("[FileSystem] Directory not found: '{0}'", virtualPath.string());
            return false;
        }
        return std::filesystem::is_directory(physicalPath);
    }

    bool FileSystem::createFile(const std::filesystem::path& virtualPath) {
        std::unique_lock lock(m_mutex);
        auto [alias, rel] = splitVirtual(virtualPath);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == alias) {
                std::filesystem::path physicalPath = mount.physicalRoot / rel;
                std::error_code ec;
                std::filesystem::create_directories(physicalPath.parent_path(), ec);
                std::ofstream file(physicalPath, std::ios::binary | std::ios::trunc);
                if (!file) {
                    TE_LOGGER_ERROR("[FileSystem] Failed to open file for writing: '{0}' (resolved to '{1}')", virtualPath.string(), physicalPath.string());
                }
            }
        }
        return true;
    }

    bool FileSystem::deleteFile(const std::filesystem::path& virtualPath) {
        std::unique_lock lock(m_mutex);
        auto [alias, rel] = splitVirtual(virtualPath);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == alias) {
                std::filesystem::path physicalPath = mount.physicalRoot / rel;
                std::error_code ec;
                std::filesystem::remove(physicalPath, ec);
                return !ec;
            }
        }
        return false;
    }

    bool FileSystem::deletePaths(const std::vector<std::filesystem::path>& virtualPaths) {
        bool allSucceeded = true;
        for (const std::filesystem::path& virtualPath: virtualPaths) {
            if (isDirectory(virtualPath)) {
                if (!deleteDirectory(virtualPath)) {
                    TE_LOGGER_ERROR("[FileSystem] deletePaths: failed to delete directory '{0}'", virtualPath.string());
                    allSucceeded = false;
                }
            } else {
                if (!deleteFile(virtualPath)) {
                    TE_LOGGER_ERROR("[FileSystem] deletePaths: failed to delete file '{0}'", virtualPath.string());
                    allSucceeded = false;
                }
            }
        }
        return allSucceeded;
    }

    bool FileSystem::copyFile(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) {
        std::unique_lock lock(m_mutex);
        auto [sourceAlias, sourceRel] = splitVirtual(virtualSource);
        auto [destAlias, destRel] = splitVirtual(virtualDestination);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == sourceAlias && mount.alias == destAlias) {
                std::filesystem::path sourcePhysicalPath = mount.physicalRoot / sourceRel;
                std::filesystem::path destPhysicalPath = mount.physicalRoot / destRel;
                std::error_code ec;
                std::filesystem::copy_file(sourcePhysicalPath, destPhysicalPath, ec);
                return !ec;
            }
        }
        return false;
    }

    bool FileSystem::moveFile(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) {
        std::unique_lock lock(m_mutex);
        if (!copyFile(virtualSource, virtualDestination)) return false;
        return deleteFile(virtualSource);
    }

    bool FileSystem::renameFile(const std::filesystem::path& virtualSource, const std::string& newFileName) {
        std::unique_lock lock(m_mutex);
        auto [sourceAlias, sourceRel] = splitVirtual(virtualSource);
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == sourceAlias) {
                std::filesystem::path sourcePhysicalPath = mount.physicalRoot / sourceRel;
                std::filesystem::path destPhysicalPath = sourcePhysicalPath.parent_path() / newFileName;
                std::error_code ec;
                std::filesystem::rename(sourcePhysicalPath, destPhysicalPath, ec);
                return !ec;
            }
        }
        return false;
    }

    std::vector<std::filesystem::path> FileSystem::candidates(const std::filesystem::path& virtualPath) {
        auto [alias, relative] = splitVirtual(virtualPath);

        std::vector<std::filesystem::path> result;
        for (const MountEntry& mount: m_mountEntries) {
            if (mount.alias == alias) {
                if (virtualPath == alias) {
                    result.emplace_back(mount.physicalRoot.string());
                } else {
                    result.emplace_back(mount.physicalRoot.string() + "/" + relative.string());
                }
            }
        }
        return result;
    }

    std::filesystem::path FileSystem::resolveInternal(const std::filesystem::path& virtualPath) {
        for (const auto& candidate: candidates(virtualPath)) {
            //if (std::filesystem::exists(candidate)) {
            return candidate;
            //}
        }
        return {};
    }

    std::pair<std::filesystem::path, std::filesystem::path> FileSystem::splitVirtual(const std::filesystem::path& virtualPath) {
        const std::string str = virtualPath.generic_string();
        const size_t pos = str.find('://');

        if (pos == std::string::npos) {
            TE_LOGGER_WARN("[FileSystem] Invalid virtual path (missing '://'): '{0}'", virtualPath.string());
            return {std::filesystem::path(), virtualPath};
        }

        std::filesystem::path alias = str.substr(0, pos + 2);
        if (pos + 2 >= str.size()) {
            return {alias, ""};
        } else {
            std::filesystem::path rel = str.substr(pos + 2);
            return {alias, rel};
        }
    }
}
