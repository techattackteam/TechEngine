#include "ResourcePathIndex.hpp"

#include "TechEngine/core/fileSystem/IFileSystem.hpp"
#include "core/Parallel.hpp"

#include <fstream>

namespace TechEngine {
    bool ResourcePathIndex::readHeaderUUID(IFileSystem& fileSystem, const std::filesystem::path& virtualPath, UUID& outUUID) {
        const std::filesystem::path physicalPath = fileSystem.resolve(virtualPath);
        if (physicalPath.empty()) {
            return false;
        }

        std::ifstream stream(physicalPath, std::ios::binary);
        if (!stream.is_open()) {
            return false;
        }

        uint64_t raw = 0;
        stream.read(reinterpret_cast<char*>(&raw), sizeof(uint64_t));
        if (stream.gcount() != static_cast<std::streamsize>(sizeof(uint64_t))) {
            return false;
        }

        outUUID = UUID(raw);
        return true;
    }

    void ResourcePathIndex::buildParallel(IFileSystem& fileSystem, const std::vector<std::filesystem::path>& resourceFiles) {
        const size_t count = resourceFiles.size();
        std::vector<UUID> uuids(count, UUID(static_cast<uint64_t>(-1)));
        std::vector<char> ok(count, 0);

        parallelFor(count, [&](size_t i) {
            UUID uuid;
            if (readHeaderUUID(fileSystem, resourceFiles[i], uuid)) {
                uuids[i] = uuid;
                ok[i] = 1;
            }
        });

        std::unique_lock lock(m_mutex);
        m_uuidToPath.clear();
        m_pathToUUID.clear();
        for (size_t i = 0; i < count; ++i) {
            if (!ok[i]) {
                continue;
            }
            m_uuidToPath[uuids[i]] = resourceFiles[i];
            m_pathToUUID[resourceFiles[i].generic_string()] = uuids[i];
        }
    }

    std::filesystem::path ResourcePathIndex::getPath(const UUID& uuid) const {
        std::shared_lock lock(m_mutex);
        auto it = m_uuidToPath.find(uuid);
        if (it != m_uuidToPath.end()) {
            return it->second;
        }
        return {};
    }

    UUID ResourcePathIndex::getUUID(const std::filesystem::path& virtualPath) const {
        std::shared_lock lock(m_mutex);
        auto it = m_pathToUUID.find(virtualPath.generic_string());
        if (it != m_pathToUUID.end()) {
            return it->second;
        }
        return UUID(static_cast<uint64_t>(-1));
    }

    bool ResourcePathIndex::contains(const UUID& uuid) const {
        std::shared_lock lock(m_mutex);
        return m_uuidToPath.find(uuid) != m_uuidToPath.end();
    }

    void ResourcePathIndex::set(const UUID& uuid, const std::filesystem::path& virtualPath) {
        if (uuid.isNull()) {
            return;
        }
        std::unique_lock lock(m_mutex);
        // Drop any stale path previously mapped to this UUID so the reverse map cannot leak entries.
        auto existing = m_uuidToPath.find(uuid);
        if (existing != m_uuidToPath.end() && existing->second != virtualPath) {
            m_pathToUUID.erase(existing->second.generic_string());
        }
        m_uuidToPath[uuid] = virtualPath;
        m_pathToUUID[virtualPath.generic_string()] = uuid;
    }

    void ResourcePathIndex::removeByPath(const std::filesystem::path& virtualPath) {
        std::unique_lock lock(m_mutex);
        auto it = m_pathToUUID.find(virtualPath.generic_string());
        if (it == m_pathToUUID.end()) {
            return;
        }
        m_uuidToPath.erase(it->second);
        m_pathToUUID.erase(it);
    }

    void ResourcePathIndex::clear() {
        std::unique_lock lock(m_mutex);
        m_uuidToPath.clear();
        m_pathToUUID.clear();
    }
}
