#pragma once

#include "serialization/buffer.hpp"
#include <filesystem>

namespace TechEngine {
    struct FileStatus {
        std::filesystem::path alias;
        std::filesystem::path virtualPath;
        std::filesystem::path name;
        std::filesystem::path extension;
        bool exists = false;
        bool isDirectory = false;
        uint64_t size = 0;
        uint64_t lastModified = 0;
    };

    class IFileSystem {
    public:
        virtual ~IFileSystem() = default;

        // mount() allows you to map a virtual path to a physical path on the disk.
        // This is useful for organizing resources and providing a consistent interface for accessing files,
        // regardless of their actual location on the filesystem.
        virtual void mount(const std::filesystem::path& virtualPath, const std::filesystem::path& physicalPath, int priority = 0) = 0;

        virtual void unmount(const std::filesystem::path& virtualPath) = 0;

        virtual bool read(const std::filesystem::path& virtualPath, Buffer& outBuffer) = 0;

        virtual bool write(const std::filesystem::path& virtualPath, const Buffer& buffer) = 0;

        virtual FileStatus status(const std::filesystem::path& virtualPath) = 0;

        virtual std::filesystem::path resolve(const std::filesystem::path& virtualPath) = 0;

        virtual std::vector<std::filesystem::path> list(const std::filesystem::path& virtualPath, bool recursive) = 0;

        virtual bool createDirectory(const std::filesystem::path& virtualPath) = 0;

        virtual bool deleteDirectory(const std::filesystem::path& virtualPath) = 0;

        virtual bool copyDirectory(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) = 0;

        virtual bool moveDirectory(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) = 0;

        virtual bool renameDirectory(const std::filesystem::path& virtualSource, const std::string& virtualDestination) = 0;

        virtual bool isDirectory(const std::filesystem::path& virtualPath) = 0;

        virtual std::filesystem::path getParentPath(const std::filesystem::path& virtualPath) = 0;

        virtual bool createFile(const std::filesystem::path& virtualPath) = 0;

        virtual bool deleteFile(const std::filesystem::path& virtualPath) = 0;

        virtual bool deletePaths(const std::vector<std::filesystem::path>& virtualPaths) = 0;

        virtual bool copyFile(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) = 0;

        virtual bool moveFile(const std::filesystem::path& virtualSource, const std::filesystem::path& virtualDestination) = 0;

        virtual bool renameFile(const std::filesystem::path& virtualSource, const std::string& newFileName) = 0;
    };
}
