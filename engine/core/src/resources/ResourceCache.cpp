#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
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
    ResourceCache::ResourceCache(JobSystem& jobs) : m_jobs(&jobs), m_slots(std::make_unique<ResourceSlots>()) {
    }

    ResourceCache::~ResourceCache() {
        shutdown();
    }

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

    bool ResourceCache::entryError(ResourceSlotId slot, std::error_code& out) const {
        const ResourceEntry* entry = m_slots->find(slot);
        if (entry == nullptr) {
            return false;
        }

        out = entry->error;
        return true;
    }

    void ResourceCache::submitLoads() {
        std::vector<ResourceSlotId> pending;
        pending.swap(m_pendingLoads);

        std::vector<Task> tasks;
        tasks.reserve(pending.size());
        for (const ResourceSlotId slot: pending) {
            ResourceEntry* entry = m_slots->find(slot);
            if (entry == nullptr || entry->state != ResourceState::Pending || entry->loading) {
                continue;
            }

            entry->loading = true;
            tasks.emplace_back([this, entry, slot] {
                runLoadTask(*entry, slot);
            });
        }

        const BatchId batch = m_jobs->submit(tasks);
        if (batch.valid()) {
            m_inFlightBatches.push_back(batch);
        }
    }

    std::error_code ResourceCache::removeCompletions() {
        std::vector<LoadCompletion> completions;
        {
            std::lock_guard<std::mutex> lock(m_completionMutex);
            completions.swap(m_completions);
        }
        std::error_code firstError;
        for (const LoadCompletion& completion: completions) {
            const std::error_code publishError = publishCompletion(completion);
            if (!firstError && publishError) {
                firstError = publishError;
            }
        }
        return firstError;
    }

    std::error_code ResourceCache::publishCompletion(const LoadCompletion& completion) {
        ResourceEntry* entry = m_slots->find(completion.slot);
        if (entry == nullptr) {
            return {};
        }

        entry->loading = false;
        if (entry->error) {
            entry->state = ResourceState::Failed;
            return entry->error;
        }

        entry->state = ResourceState::Ready;
        for (const ResourceId dependency: completion.dependencies) {
            ResourceSlotId dependencySlot;
            const std::error_code dependencyError = requestEntry(dependency, StringId{}, dependencySlot);
            if (dependencyError) {
                entry->state = ResourceState::Failed;
                entry->error = dependencyError;
                return dependencyError;
            }
            if (reaches(dependencySlot, completion.slot)) {
                releaseEntry(dependencySlot);
                entry->state = ResourceState::Failed;
                entry->error = ReadError::CyclicReference;
                return entry->error;
            }
            entry->dependencies.push_back(dependencySlot);
        }

        return {};
    }

    void ResourceCache::runLoadTask(ResourceEntry& entry, const ResourceSlotId slot) {
        std::vector<std::byte> bytes;
        std::vector<ResourceId> dependencies;
        std::error_code loadError = m_source->read(entry.id, bytes);
        if (!loadError) {
            const TypeRecord* record = findTypeForBytes(bytes);
            if (record == nullptr) {
                loadError = ReadError::BadMagic;
            } else {
                TE_CHECK(entry.typeTag == StringId{} || entry.typeTag == record->typeTag, "Resource {0:#018x} was requested as {1}, but it holds {2}", entry.id.value(), entry.typeTag, record->typeTag);
                void* payload = record->createPayload();
                loadError = record->load(bytes, payload, dependencies);
                if (loadError) {
                    record->destroyPayload(payload);
                } else {
                    entry.payload = std::unique_ptr<void, ResourcePayloadDeleter>{payload, record->destroyPayload};
                    entry.typeTag = record->typeTag;
                }
            }
        }

        entry.error = loadError;
        {
            const std::lock_guard lock{m_completionMutex};
            m_completions.push_back(LoadCompletion{slot, std::move(dependencies)});
        }
    }

    void ResourceCache::shutdown() {
        for (const BatchId batch: m_inFlightBatches) {
            m_jobs->wait(batch);
        }
        m_inFlightBatches.clear();

        for (const auto& [id, slot]: m_byId) {
            const ResourceEntry* entry = m_slots->find(slot);
            if (entry != nullptr && entry->acquires > 0) {
                TE_LOGGER_WARN("Resource {0:#018x} was not released before cache shutdown", id.value());
            }
        }
    }

    std::error_code ResourceCache::runLoadingPhase() {
        std::error_code firstError;
        while (true) {
            submitLoads();
            for (const BatchId batch: m_inFlightBatches) {
                m_jobs->wait(batch);
            }
            m_inFlightBatches.clear();

            const std::error_code completionError = removeCompletions();
            if (!firstError && completionError) {
                firstError = completionError;
            }

            bool loading = false;
            for (const auto& [id, slot]: m_byId) {
                const ResourceEntry* entry = m_slots->find(slot);
                if (entry != nullptr && entry->loading) {
                    loading = true;
                    break;
                }
            }
            if (!loading && m_pendingLoads.empty()) {
                break;
            }
        }

        drainFrees();
        return firstError;
    }

    void ResourceCache::drainFrees() {
        std::vector<ResourceSlotId> deferred;
        while (!m_pendingFrees.empty()) {
            std::vector<ResourceSlotId> draining;
            draining.swap(m_pendingFrees);
            for (const ResourceSlotId slot: draining) {
                ResourceEntry* entry = m_slots->find(slot);
                if (entry == nullptr) {
                    continue;
                }
                if (entry->loading || entry->acquires > 0) {
                    deferred.push_back(slot);
                    continue;
                }
                freeEntry(slot);
            }
        }
        m_pendingFrees.insert(m_pendingFrees.end(), deferred.begin(), deferred.end());
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
