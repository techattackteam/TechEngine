#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>

#include <resources/ResourceSlots.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace TechEngine {
    ResourceCache::ResourceCache() : m_slots(std::make_unique<ResourceSlots>()) {
    }

    ResourceCache::~ResourceCache() = default;

    void ResourceCache::setSource(const AssetSource& source) {
        m_source = &source;
    }

    void ResourceCache::closeLoaders() {
        m_loadersClosed = true;
    }

    bool ResourceCache::loadersClosed() const {
        return m_loadersClosed;
    }

    void ResourceCache::registerType(const BlobKind& kind, const StringId typeTag, const StringId existingTypeTag, const ResourceLoadFn load, void* (*createPayload)(), void (*destroyPayload)(void*)) {
        TE_CHECK(!m_loadersClosed, "Resource loader registration is closed - {0} is registered after the cache closed it", typeTag);
        TE_CHECK(typeTag != StringId{}, "A resource type tag must not be null");
        TE_CHECK(load != nullptr, "Resource type {0} registers without a loader", typeTag);
        TE_CHECK(existingTypeTag == StringId{} || existingTypeTag == typeTag, "This resource type is already registered under a different tag: {0}", typeTag);

        for (const TypeRecord& record: m_types) {
            TE_CHECK(record.typeTag != typeTag, "Resource type tag already registered: {0}", typeTag);
            TE_CHECK(record.kind.magic != kind.magic, "Resource magic {0:#010x} is already registered under another tag", kind.magic);
        }
        m_types.push_back(TypeRecord{kind, typeTag, load, createPayload, destroyPayload});
    }

    std::error_code ResourceCache::requestEntry(const ResourceId id, const StringId typeTag, ResourceSlotId& out) {
        TE_CHECK(m_source != nullptr, "A resource cache needs an asset source before its first request");
        TE_CHECK(id.valid(), "A null resource id is never requested");

        const auto known = m_byId.find(id);
        if (known != m_byId.end()) {
            ResourceEntry* entry = m_slots->find(known->second);
            TE_ASSERT(entry != nullptr, "Resource {0:#018x} is indexed on a slot that no longer exists", id.value());
            entry->acquires++;
            out = known->second;
            return {};
        }

        const AssetAvailability availability = m_source->find(id);
        if (availability == AssetAvailability::Unknown) {
            return ReadError::UnknownResource;
        }

        const ResourceSlotId slot = m_slots->create();
        ResourceEntry* entry = m_slots->find(slot);
        TE_ASSERT(entry != nullptr, "A slot does not resolve right after it was created");
        entry->id = id;
        entry->typeTag = typeTag;
        entry->acquires = 1;
        if (availability == AssetAvailability::AbsentForRole) {
            entry->state = ResourceState::AbsentForRole;
        } else {
            entry->state = ResourceState::Pending;
            m_pendingLoads.push_back(slot);
        }
        m_byId.emplace(id, slot);
        out = slot;
        return {};
    }

    void ResourceCache::releaseEntry(const ResourceSlotId slot) {
        ResourceEntry* entry = m_slots->find(slot);
        if (entry == nullptr) {
            return;
        }

        TE_CHECK(entry->acquires > 0, "Resource {0:#018x} is released more often than it was acquired", entry->id.value());
        entry->acquires--;
        if (entry->acquires == 0) {
            m_pendingFrees.push_back(slot);
        }
    }

    const void* ResourceCache::resolvePayload(const ResourceSlotId slot, const StringId typeTag) const {
        const ResourceEntry* entry = m_slots->find(slot);
        if (entry == nullptr || entry->state != ResourceState::Ready) {
            return nullptr;
        }

        TE_CHECK(entry->typeTag == typeTag, "Resource {0:#018x} is resolved as {1}, but it was loaded as {2}", entry->id.value(), typeTag, entry->typeTag);
        return entry->payload.get();
    }

    bool ResourceCache::entryState(const ResourceSlotId slot, ResourceState& out) const {
        const ResourceEntry* entry = m_slots->find(slot);
        if (entry == nullptr) {
            return false;
        }

        out = entry->state;
        return true;
    }

    bool ResourceCache::entryAcquireCount(const ResourceSlotId slot, std::uint32_t& out) const {
        const ResourceEntry* entry = m_slots->find(slot);
        if (entry == nullptr) {
            return false;
        }

        out = entry->acquires;
        return true;
    }

    std::error_code ResourceCache::runLoadingPhase() {
        std::vector<std::byte> bytes;
        std::vector<ResourceId> dependencies;

        // Loading an entry appends its dependencies to m_pendingLoads, so the index walks on
        // into them as the next round and the phase ends only when nothing is left pending.
        std::size_t next = 0;
        while (next < m_pendingLoads.size()) {
            const ResourceSlotId slot = m_pendingLoads[next];
            next++;
            const std::error_code loadError = loadEntry(slot, bytes, dependencies);
            if (loadError) {
                m_pendingLoads.erase(m_pendingLoads.begin(), m_pendingLoads.begin() + static_cast<std::ptrdiff_t>(next));
                return loadError;
            }
        }

        m_pendingLoads.clear();
        drainFrees();
        return {};
    }

    void ResourceCache::drainFrees() {
        // Freeing an entry releases its dependencies, which queues more frees, so a whole tree
        // goes in one drain instead of one level per phase.
        while (!m_pendingFrees.empty()) {
            std::vector<ResourceSlotId> draining;
            draining.swap(m_pendingFrees);
            for (const ResourceSlotId slot: draining) {
                freeEntry(slot);
            }
        }
    }

    std::error_code ResourceCache::loadEntry(const ResourceSlotId slot, std::vector<std::byte>& bytes, std::vector<ResourceId>& dependencies) {
        ResourceEntry* entry = m_slots->find(slot);
        if (entry == nullptr || entry->state != ResourceState::Pending) {
            return {};
        }

        bytes.clear();
        const std::error_code readError = m_source->read(entry->id, bytes);
        if (readError) {
            entry->state = ResourceState::Failed;
            return readError;
        }

        const TypeRecord* record = findTypeForBytes(bytes);
        if (record == nullptr) {
            entry->state = ResourceState::Failed;
            return ReadError::BadMagic;
        }
        TE_CHECK(entry->typeTag == StringId{} || entry->typeTag == record->typeTag, "Resource {0:#018x} was requested as {1}, but it holds {2}", entry->id.value(), entry->typeTag, record->typeTag);

        dependencies.clear();
        void* payload = record->createPayload();
        const std::error_code loadError = record->load(bytes, payload, dependencies);
        if (loadError) {
            record->destroyPayload(payload);
            entry->state = ResourceState::Failed;
            return loadError;
        }

        entry->payload = std::unique_ptr<void, ResourcePayloadDeleter>{payload, record->destroyPayload};
        entry->typeTag = record->typeTag;
        entry->state = ResourceState::Ready;

        for (const ResourceId dependency: dependencies) {
            ResourceSlotId dependencySlot;
            const std::error_code dependencyError = requestEntry(dependency, StringId{}, dependencySlot);
            if (dependencyError) {
                entry->state = ResourceState::Failed;
                return dependencyError;
            }
            if (reaches(dependencySlot, slot)) {
                releaseEntry(dependencySlot);
                entry->state = ResourceState::Failed;
                return ReadError::CyclicReference;
            }
            entry->dependencies.push_back(dependencySlot);
        }
        return {};
    }

    void ResourceCache::freeEntry(const ResourceSlotId slot) {
        ResourceEntry* entry = m_slots->find(slot);
        if (entry == nullptr || entry->acquires > 0) {
            return;
        }

        const std::vector<ResourceSlotId> dependencies = std::move(entry->dependencies);
        m_byId.erase(entry->id);
        m_slots->destroy(slot);
        for (const ResourceSlotId dependency: dependencies) {
            releaseEntry(dependency);
        }
    }

    bool ResourceCache::reaches(const ResourceSlotId from, const ResourceSlotId target) const {
        std::vector<ResourceSlotId> pending;
        std::unordered_set<std::uint32_t> visited;
        pending.push_back(from);
        while (!pending.empty()) {
            const ResourceSlotId slot = pending.back();
            pending.pop_back();
            if (slot == target) {
                return true;
            }
            if (!visited.insert(slot.index).second) {
                continue;
            }

            const ResourceEntry* entry = m_slots->find(slot);
            if (entry == nullptr) {
                continue;
            }
            for (const ResourceSlotId dependency: entry->dependencies) {
                pending.push_back(dependency);
            }
        }
        return false;
    }

    const ResourceCache::TypeRecord* ResourceCache::findTypeForBytes(const std::span<const std::byte> bytes) const {
        Reader reader{bytes};
        std::uint32_t magic = 0;
        reader.read(magic);
        if (!reader.ok()) {
            return nullptr;
        }

        for (const TypeRecord& record: m_types) {
            if (record.kind.magic == magic) {
                return &record;
            }
        }
        return nullptr;
    }
}
