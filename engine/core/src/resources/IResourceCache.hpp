#pragma once

#include <TechEngine/core/core/UUID.hpp>

#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace TechEngine {
    class IResource;

    template<typename T>
    class IResourceCache {
        static_assert(std::is_base_of_v<IResource, T>, "ResourceCache<T>: T must derive from IResource");

    private:
        mutable std::shared_mutex m_mutex;
        std::unordered_map<std::string, std::shared_ptr<T>> m_byName;
        std::unordered_map<UUID, std::shared_ptr<T>> m_byUUID;

    public:
        bool add(std::shared_ptr<T> resource) {
            if (!resource) {
                return false;
            }
            std::unique_lock lock(m_mutex);
            const std::string& name = resource->getName();
            if (m_byName.find(name) != m_byName.end()) {
                return false; // Resource with the same name already exists
            }
            const UUID& uuid = resource->getUUID();
            m_byUUID[uuid] = resource;
            m_byName[name] = resource;
            return true;
        }

        void replace(std::shared_ptr<T> resource) {
            if (!resource) {
                return;
            }
            std::unique_lock lock(m_mutex);
            const UUID& uuid = resource->getUUID();

            auto it = m_byUUID.find(uuid);
            if (it != m_byUUID.end()) {
                m_byName.erase(it->second->getName());
            }

            const std::string& name = resource->getName();
            m_byName[name] = resource;
            m_byUUID[uuid] = resource;
        }

        std::shared_ptr<T> get(const std::string& name) const {
            std::shared_lock lock(m_mutex);
            auto it = m_byName.find(name);
            if (it != m_byName.end()) {
                return it->second;
            }
            return nullptr; // Resource not found
        }

        std::shared_ptr<T> get(const UUID& uuid) const {
            std::shared_lock lock(m_mutex);
            auto it = m_byUUID.find(uuid);
            if (it != m_byUUID.end()) {
                return it->second;
            }
            return nullptr; // Resource not found
        }

        bool contains(const std::string& name) const {
            std::shared_lock lock(m_mutex);
            return m_byName.find(name) != m_byName.end();
        }

        bool contains(const UUID& uuid) const {
            std::shared_lock lock(m_mutex);
            return m_byUUID.find(uuid) != m_byUUID.end();
        }

        bool remove(const std::string& name) {
            std::unique_lock lock(m_mutex);
            auto it = m_byName.find(name);
            if (it != m_byName.end()) {
                m_byUUID.erase(it->second->getUUID());
                m_byName.erase(it);
                return true;
            }
            return false; // Resource not found
        }

        bool remove(const UUID& uuid) {
            std::unique_lock lock(m_mutex);
            auto it = m_byUUID.find(uuid);
            if (it != m_byUUID.end()) {
                m_byName.erase(it->second->getName());
                m_byUUID.erase(it);
                return true;
            }
            return false;
        }

        size_t count() const {
            std::shared_lock lock(m_mutex);
            return m_byName.size();
        }

        void clear() {
            std::unique_lock lock(m_mutex);
            m_byName.clear();
            m_byUUID.clear();
        }

        std::vector<std::string> getNames() const {
            std::shared_lock lock(m_mutex);
            std::vector<std::string> names;
            names.reserve(m_byName.size());
            for (const auto& [name, _] : m_byName) {
                names.push_back(name);
            }
            return names;
        }

        std::vector<std::shared_ptr<T>> getAll() const {
            std::shared_lock lock(m_mutex);
            std::vector<std::shared_ptr<T>> resources;
            resources.reserve(m_byUUID.size());
            for (const auto& [uuid, resource] : m_byUUID) {
                resources.push_back(resource);
            }
            return resources;
        }
    };
}
