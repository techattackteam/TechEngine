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
        waitForLoads();
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

    void ResourceCache::markUploadedType(StringId typeTag) {
        TE_CHECK(!m_loadersClosed, "Resource loader registration is closed - cannot mark types as uploaded");
        for (TypeRecord& record: m_types) {
            if (record.typeTag == typeTag) {
                record.uploaded = true;
                return;
            }
        }
        TE_CHECK(false, "Resource type tag not found: {0}", typeTag);
    }

    bool ResourceCache::isUploaded(StringId typeTag) const {
        for (const TypeRecord& record: m_types) {
            if (record.typeTag == typeTag) {
                return record.uploaded;
            }
        }
        return false;
    }

    std::error_code ResourceCache::requestEntry(const ResourceId id, const StringId typeTag, const ResourceSlotId holder, ResourceSlotId& out) {
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
            if (!holder.valid()) {
                return ReadError::UnknownResource;
            }
            const ResourceEntry* holderEntry = m_slots->find(holder);
            TE_ASSERT(holderEntry != nullptr, "Resource {0:#018x} is requested for a holder slot that no longer exists", id.value());
            TE_LOGGER_WARN("Resource {0:#018x} references {1:#018x}, which no file holds, so it loads as missing", holderEntry->id.value(), id.value());
        }

        const ResourceSlotId slot = m_slots->create();
        ResourceEntry* entry = m_slots->find(slot);
        TE_ASSERT(entry != nullptr, "A slot does not resolve right after it was created");
        entry->id = id;
        entry->typeTag = typeTag;
        entry->acquires = 1;
        if (availability == AssetAvailability::AbsentForRole) {
            entry->state = ResourceState::AbsentForRole;
        } else if (availability == AssetAvailability::Unknown) {
            entry->state = ResourceState::Missing;
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

    void* ResourceCache::resolvePayload(const ResourceSlotId slot, const StringId typeTag) {
        return const_cast<void*>(std::as_const(*this).resolvePayload(slot, typeTag));
    }

    std::error_code ResourceCache::addEntryDependency(const ResourceSlotId holder, const ResourceId dependency, ResourceSlotId& out) {
        ResourceEntry* entry = m_slots->find(holder);
        TE_CHECK(entry != nullptr, "A dependency is added to a resource slot that no longer exists");
        TE_CHECK(entry->state == ResourceState::Ready, "Resource {0:#018x} gets a dependency before it is Ready", entry->id.value());
        const std::error_code dependencyError = acquireDependency(holder, dependency, out);
        if (dependencyError) {
            return dependencyError;
        }
        entry->dependencies.push_back(out);
        return {};
    }

    std::error_code ResourceCache::acquireDependency(const ResourceSlotId holder, const ResourceId dependency, ResourceSlotId& out) {
        const std::error_code requestError = requestEntry(dependency, StringId{}, holder, out);
        if (requestError) {
            return requestError;
        }
        if (reaches(out, holder)) {
            releaseEntry(out);
            return ReadError::CyclicReference;
        }
        return {};
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
                runLoadTask(*entry, slot, false);
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
        const auto settle = [this, &firstError](const LoadCompletion& completion, const std::error_code error) {
            if (completion.reload && error) {
                const ResourceEntry* entry = m_slots->find(completion.slot);
                TE_LOGGER_WARN("Resource {0:#018x} keeps its previous data, because its reload failed: {1}", entry->id.value(), error.message());
            }
            if (!firstError && error) {
                firstError = error;
            }
        };

        // A reload is checked for cycles against the old edges of the entries reloaded beside it, so one refused as a
        // cycle is tried again after the others are published, until a round settles none of them.
        std::vector<LoadCompletion*> cyclicReloads;
        for (LoadCompletion& completion: completions) {
            const std::error_code publishError = publishCompletion(completion);
            if (completion.reload && publishError == ReadError::CyclicReference) {
                cyclicReloads.push_back(&completion);
                continue;
            }
            settle(completion, publishError);
        }
        bool settledAny = true;
        while (settledAny && !cyclicReloads.empty()) {
            settledAny = false;
            std::vector<LoadCompletion*> stillCyclic;
            for (LoadCompletion* completion: cyclicReloads) {
                const std::error_code retryError = publishCompletion(*completion);
                if (retryError == ReadError::CyclicReference) {
                    stillCyclic.push_back(completion);
                    continue;
                }
                settledAny = true;
                settle(*completion, retryError);
            }
            cyclicReloads = std::move(stillCyclic);
        }
        for (const LoadCompletion* completion: cyclicReloads) {
            settle(*completion, ReadError::CyclicReference);
        }
        return firstError;
    }

    std::error_code ResourceCache::publishCompletion(LoadCompletion& completion) {
        ResourceEntry* entry = m_slots->find(completion.slot);
        if (entry == nullptr) {
            return {};
        }

        entry->loading = false;
        if (completion.reload) {
            return publishReload(*entry, completion);
        }

        entry->error = completion.error;
        if (entry->error) {
            entry->state = ResourceState::Failed;
            return entry->error;
        }

        entry->payload = std::move(completion.payload);
        entry->typeTag = completion.typeTag;
        entry->state = ResourceState::Ready;
        for (const ResourceId dependency: completion.dependencies) {
            ResourceSlotId dependencySlot;
            const std::error_code dependencyError = addEntryDependency(completion.slot, dependency, dependencySlot);
            if (dependencyError) {
                entry->state = ResourceState::Failed;
                entry->error = dependencyError;
                return dependencyError;
            }
        }

        recordUpload(completion.slot, *entry);
        return {};
    }

    std::error_code ResourceCache::publishReload(ResourceEntry& entry, LoadCompletion& completion) {
        if (completion.error) {
            return completion.error;
        }

        std::vector<ResourceSlotId> dependencies;
        dependencies.reserve(completion.dependencies.size());
        for (const ResourceId dependency: completion.dependencies) {
            ResourceSlotId dependencySlot;
            const std::error_code dependencyError = acquireDependency(completion.slot, dependency, dependencySlot);
            if (dependencyError) {
                for (const ResourceSlotId acquired: dependencies) {
                    releaseEntry(acquired);
                }
                return dependencyError;
            }
            dependencies.push_back(dependencySlot);
        }

        std::unique_ptr<void, ResourcePayloadDeleter> previous = std::exchange(entry.payload, std::move(completion.payload));
        retirePayload(entry, std::move(previous));
        entry.typeTag = completion.typeTag;
        entry.error = {};
        entry.state = ResourceState::Ready;
        const std::vector<ResourceSlotId> previousDependencies = std::exchange(entry.dependencies, std::move(dependencies));
        for (const ResourceSlotId dependency: previousDependencies) {
            releaseEntry(dependency);
        }
        recordUpload(completion.slot, entry);
        return {};
    }

    void ResourceCache::runLoadTask(const ResourceEntry& entry, const ResourceSlotId slot, const bool reload) {
        LoadCompletion completion;
        completion.slot = slot;
        completion.reload = reload;
        std::vector<std::byte> bytes;
        std::error_code loadError = m_source->read(entry.id, bytes);
        if (!loadError) {
            const TypeRecord* record = findTypeForBytes(bytes);
            const bool kindChanged = record != nullptr && reload && entry.typeTag != StringId{} && entry.typeTag != record->typeTag;
            if (record == nullptr || kindChanged) {
                loadError = ReadError::BadMagic;
            } else {
                TE_CHECK(entry.typeTag == StringId{} || entry.typeTag == record->typeTag, "Resource {0:#018x} was requested as {1}, but it holds {2}", entry.id.value(), entry.typeTag, record->typeTag);
                completion.payload = std::unique_ptr<void, ResourcePayloadDeleter>{record->createPayload(), record->destroyPayload};
                loadError = record->load(bytes, completion.payload.get(), completion.dependencies);
                if (loadError) {
                    completion.payload.reset();
                } else {
                    completion.typeTag = record->typeTag;
                }
            }
        }

        completion.error = loadError;
        {
            const std::lock_guard lock{m_completionMutex};
            m_completions.push_back(std::move(completion));
        }
    }

    void ResourceCache::waitForLoads() {
        for (const BatchId batch: m_inFlightBatches) {
            m_jobs->wait(batch);
        }
        m_inFlightBatches.clear();
    }

    void ResourceCache::shutdown() {
        waitForLoads();

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
            waitForLoads();

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

    std::error_code ResourceCache::reload(const std::span<const ResourceId> ids) {
        std::vector<Task> tasks;
        tasks.reserve(ids.size());
        for (const ResourceId id: ids) {
            const auto known = m_byId.find(id);
            if (known == m_byId.end()) {
                continue;
            }

            const ResourceSlotId slot = known->second;
            ResourceEntry* entry = m_slots->find(slot);
            TE_ASSERT(entry != nullptr, "Resource {0:#018x} is indexed on a slot that no longer exists", id.value());
            const bool loaded = entry->state == ResourceState::Ready || entry->state == ResourceState::Failed;
            if (!loaded || entry->loading) {
                continue;
            }

            entry->loading = true;
            tasks.emplace_back([this, entry, slot] {
                runLoadTask(*entry, slot, true);
            });
        }

        const BatchId batch = m_jobs->submit(tasks);
        if (batch.valid()) {
            m_inFlightBatches.push_back(batch);
        }
        return runLoadingPhase();
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

                if (entry->loading || entry->acquires > 0 || entry->pendingUploadToken != 0) {
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
        recordRelease(slot, *entry);
        m_byId.erase(entry->id);
        m_slots->destroy(slot);
        for (const ResourceSlotId dependency: dependencies) {
            releaseEntry(dependency);
        }
    }

    std::error_code ResourceCache::listDependencies(const std::span<const std::byte> bytes, std::vector<ResourceId>& out) const {
        out.clear();
        const TypeRecord* record = findTypeForBytes(bytes);
        if (record == nullptr) {
            return ReadError::BadMagic;
        }
        const std::unique_ptr<void, ResourcePayloadDeleter> payload{record->createPayload(), record->destroyPayload};
        return record->load(bytes, payload.get(), out);
    }

    void ResourceCache::takeUploads(std::vector<ResourceUploadRecord>& out) {
        out.clear();
        out.swap(m_uploads);
    }

    void ResourceCache::finishUploads(const std::span<const FinishedUpload> uploads) {
        for (const FinishedUpload& upload: uploads) {
            ResourceEntry* entry = m_slots->find(upload.slot);
            if (entry != nullptr && entry->pendingUploadToken == upload.token) {
                entry->pendingUploadToken = 0;
                continue;
            }

            for (auto retired = m_retiredPayloads.begin(); retired != m_retiredPayloads.end(); retired++) {
                if (retired->token == upload.token) {
                    m_retiredPayloads.erase(retired);
                    break;
                }
            }
        }
    }

    void ResourceCache::recordUpload(const ResourceSlotId slot, ResourceEntry& entry) {
        if (!isUploaded(entry.typeTag)) {
            return;
        }

        const std::uint64_t token = m_nextUploadToken++;
        entry.pendingUploadToken = token;
        m_uploads.push_back(ResourceUploadRecord{ResourceUploadKind::Upload, slot, entry.typeTag, entry.id, entry.payload.get(), entry.dependencies, token});
    }

    void ResourceCache::recordRelease(const ResourceSlotId slot, const ResourceEntry& entry) {
        if (!isUploaded(entry.typeTag) || entry.state != ResourceState::Ready) {
            return;
        }

        ResourceUploadRecord release;
        release.kind = ResourceUploadKind::Release;
        release.slot = slot;
        release.typeTag = entry.typeTag;
        release.id = entry.id;
        m_uploads.push_back(std::move(release));
    }

    void ResourceCache::retirePayload(ResourceEntry& entry, std::unique_ptr<void, ResourcePayloadDeleter> previous) {
        if (entry.pendingUploadToken != 0) {
            m_retiredPayloads.push_back(RetiredPayload{entry.pendingUploadToken, std::move(previous)});
            entry.pendingUploadToken = 0;
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
