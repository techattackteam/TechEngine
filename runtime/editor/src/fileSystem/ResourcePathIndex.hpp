#pragma once

#include "TechEngine/core/core/UUID.hpp"

#include <filesystem>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    class IFileSystem;

    class ResourcePathIndex {
    private:
        mutable std::shared_mutex m_mutex;
        std::unordered_map<UUID, std::filesystem::path> m_uuidToPath;
        std::unordered_map<std::string, UUID> m_pathToUUID;

    public:
        static bool readHeaderUUID(IFileSystem& fileSystem, const std::filesystem::path& virtualPath, UUID& outUUID);

        void buildParallel(IFileSystem& fileSystem, const std::vector<std::filesystem::path>& resourceFiles);

        std::filesystem::path getPath(const UUID& uuid) const;

        UUID getUUID(const std::filesystem::path& virtualPath) const;

        bool contains(const UUID& uuid) const;

        void set(const UUID& uuid, const std::filesystem::path& virtualPath);

        void removeByPath(const std::filesystem::path& virtualPath);

        void clear();
    };
}
