#pragma once

#include "TechEngine/core/fileSystem/IFileSystem.hpp"
#include "systems/System.hpp"
#include <shared_mutex>

namespace TechEngine {
    struct MountEntry {
        std::filesystem::path alias;
        std::filesystem::path physicalRoot;
        int priority = 0;
    };


    class FileSystem : public System, public IFileSystem {
    public:
        FileSystem() = default;

        ~FileSystem() override = default;

        void mount(const std::filesystem::path& alias, const std::filesystem::path& physicalPath, int priority = 0) override;

        void unmount(const std::filesystem::path& virtualPath) override;

        bool read(const std::filesystem::path& virtualPath, Buffer& outBuffer) override;

        bool write(const std::filesystem::path& virtualPath, const Buffer& buffer) override;

        FileStatus status(const std::filesystem::path& virtualPath) override;

        std::filesystem::path resolve(const std::filesystem::path& virtualPath) override;

        std::vector<std::filesystem::path> list(const std::filesystem::path& virtualPath, bool recursive) override;

        bool createDirectory(const std::filesystem::path& virtualPath) override;

        bool deleteDirectory(const std::filesystem::path& virtualPath) override;

        bool copyDirectory(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) override;

        bool moveDirectory(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) override;

        bool renameDirectory(const std::filesystem::path& virtualSource, const std::string& virtualDestination) override;

        std::filesystem::path getParentPath(const std::filesystem::path& virtualPath) override;

        bool isDirectory(const std::filesystem::path& virtualPath) override;

        bool createFile(const std::filesystem::path& virtualPath) override;

        bool deleteFile(const std::filesystem::path& virtualPath) override;

        bool deletePaths(const std::vector<std::filesystem::path>& virtualPaths) override;

        bool copyFile(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) override;

        bool moveFile(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) override;

        bool renameFile(const std::filesystem::path& virtualSource, const std::string& newFileName) override;

    private:
        mutable std::shared_mutex m_mutex;
        std::vector<MountEntry> m_mountEntries;

        std::vector<std::filesystem::path> candidates(const std::filesystem::path& virtualPath);

        std::filesystem::path resolveInternal(const std::filesystem::path& virtualPath);

        std::pair<std::filesystem::path, std::filesystem::path> splitVirtual(const std::filesystem::path& virtualPath);
    };
}
