#pragma once

#include <TechEngine/core/core/UUID.hpp>

#include <unordered_map>
#include <vector>
#include <stdexcept>

#include "core/Logger.hpp"
#include "renderer/shaders/ShaderStorageBuffer.hpp"

namespace TechEngine {
    template<typename T>
    class GPUResourceCache {
    private:
        struct Entry {
            T resource;
            uint32_t ssboSlot;
        };

        std::unordered_map<UUID, Entry> m_entries;
        std::vector<uint32_t> m_freeSlots;
        uint32_t m_nextSlot = 0;
        uint32_t m_capacity = 0;

        ShaderStorageBuffer* m_ssbo;

    public:
        explicit GPUResourceCache(uint32_t capacity,
                                  ShaderStorageBuffer* ssbo) : m_capacity(capacity),
                                                               m_ssbo(ssbo) {
        }

        uint32_t add(const UUID& uuid, T&& resource) {
            if (m_entries.count(uuid)) {
                TE_LOGGER_CRITICAL("GPUResourceCache: UUID already registered: {0}", uuid.toString());
            }

            uint32_t slot = allocateSlot();
            m_entries.emplace(uuid, Entry{std::move(resource), slot});
            return slot;
        }

        T* get(const UUID& uuid) {
            auto it = m_entries.find(uuid);
            if (it != m_entries.end()) {
                return &it->second.resource;
            }
            TE_LOGGER_WARN("GPUResourceCache: Resource not found: {0}", uuid.toString());
            return nullptr;
        }

        const T* get(const UUID& uuid) const {
            auto it = m_entries.find(uuid);
            if (it != m_entries.end()) {
                return &it->second.resource;
            }
            TE_LOGGER_WARN("GPUResourceCache: Resource not found: {0}", uuid.toString());
            return nullptr;
        }

        uint32_t getSlot(const UUID& uuid) const {
            auto it = m_entries.find(uuid);
            if (it != m_entries.end()) {
                return it->second.ssboSlot;
            }
            return static_cast<uint32_t>(-1);
        }

        bool remove(const UUID& uuid) {
            auto it = m_entries.find(uuid);
            if (it == m_entries.end()) {
                return false;
            }

            m_freeSlots.push_back(it->second.ssboSlot);
            m_entries.erase(it);
            return true;
        }

        bool contains(const UUID& uuid) const {
            return m_entries.count(uuid) > 0;
        }

        bool isFull() const {
            return m_freeSlots.empty() && m_nextSlot >= m_capacity;
        }

        size_t count() const {
            return m_entries.size();
        }

        size_t capacity() const {
            return m_capacity;
        }

        template<typename Fn>
        void forEach(Fn&& fn) {
            for (auto& [uuid, entry]: m_entries) {
                fn(entry.resource, entry.ssboSlot);
            }
        }

        template<typename Fn>
        void forEach(Fn&& fn) const {
            for (const auto& [uuid, entry]: m_entries) {
                fn(entry.resource, entry.ssboSlot);
            }
        }

        void clear() {
            m_entries.clear();
            m_freeSlots.clear();
            m_nextSlot = 0;
        }

    private:
        uint32_t allocateSlot() {
            if (!m_freeSlots.empty()) {
                uint32_t slot = m_freeSlots.back();
                m_freeSlots.pop_back();
                return slot;
            }
            if (m_nextSlot >= m_capacity) {
                const uint32_t newCapacity = m_capacity * 2;
                if (m_ssbo != nullptr) {
                    m_ssbo->resize(newCapacity);
                }
                resize(m_capacity * 2);
            }
            return m_nextSlot++;
        }

        void resize(uint32_t newCapacity) {
            if (newCapacity <= m_capacity) {
                TE_LOGGER_WARN("GPUResourceCache: New capacity must be greater than current capacity.");
                return;
            }
            m_capacity = newCapacity;
        }
    };
}
