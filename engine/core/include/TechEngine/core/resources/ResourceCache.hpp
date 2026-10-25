#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TechEngine {
    class AssetSource;
    class ResourceSlots;
    struct ResourceEntry;

    enum class ResourceState : std::uint8_t { Pending, Ready, Failed, AbsentForRole };

    // A loader appends the ids its payload references to dependencies instead of loading them:
    // the cache requests them as the next round of the loading phase.
    using ResourceLoadFn = std::error_code (*)(std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies);

    namespace internal {
        template<typename T>
        inline StringId g_resourceTypeTag{};
    }

    template<typename T>
    StringId resourceTypeTag() {
        return internal::g_resourceTypeTag<T>;
    }

    class ResourceCache {
    private:
        struct TypeRecord {
            BlobKind kind;
            StringId typeTag;
            ResourceLoadFn load = nullptr;
            void* (*createPayload)() = nullptr;
            void (*destroyPayload)(void*) = nullptr;
        };

        struct LoadCompletion {
            ResourceSlotId slot;
            std::vector<ResourceId> dependencies;
        };

        JobSystem* m_jobs = nullptr;
        const AssetSource* m_source = nullptr;
        std::unique_ptr<ResourceSlots> m_slots;
        std::unordered_map<ResourceId, ResourceSlotId> m_byId;
        std::vector<TypeRecord> m_types;
        std::vector<ResourceSlotId> m_pendingLoads;
        std::vector<ResourceSlotId> m_pendingFrees;
        std::vector<BatchId> m_inFlightBatches;
        std::mutex m_completionMutex;
        std::vector<LoadCompletion> m_completions;
        bool m_loadersClosed = false;

    public:
        explicit ResourceCache(JobSystem& jobs);

        ~ResourceCache();

        ResourceCache(const ResourceCache&) = delete;

        ResourceCache& operator=(const ResourceCache&) = delete;

        void setSource(const AssetSource& source);

        template<typename T>
        void registerLoader(const BlobKind& kind, const StringId typeTag, const ResourceLoadFn load) {
            registerType(kind, typeTag, internal::g_resourceTypeTag<T>, load, &createResourcePayload<T>, &destroyResourcePayload<T>);
            internal::g_resourceTypeTag<T> = typeTag;
        }

        void closeLoaders();

        bool loadersClosed() const;

        template<typename T>
        std::error_code request(const ResourceId id, ResourceHandle<T>& out) {
            ResourceSlotId slot;
            const std::error_code requestError = requestEntry(id, resourceTypeTag<T>(), slot);
            out = ResourceHandle<T>{slot.index, slot.generation};
            return requestError;
        }

        template<typename T>
        void release(const ResourceHandle<T> handle) {
            releaseEntry(handle.slot());
        }

        template<typename T>
        const T* resolve(const ResourceHandle<T> handle) const {
            return static_cast<const T*>(resolvePayload(handle.slot(), resourceTypeTag<T>()));
        }

        template<typename T>
        bool getState(const ResourceHandle<T> handle, ResourceState& out) const {
            return entryState(handle.slot(), out);
        }

        template<typename T>
        bool getAcquireCount(const ResourceHandle<T> handle, std::uint32_t& out) const {
            return entryAcquireCount(handle.slot(), out);
        }

        template<typename T>
        bool getError(const ResourceHandle<T> handle, std::error_code& out) const {
            return entryError(handle.slot(), out);
        }

        template<typename T>
        std::error_code addDependency(const ResourceHandle<T> holder, const ResourceId dependency, ResourceSlotId& out) {
            return addEntryDependency(holder.slot(), dependency, out);
        }

        template<typename T>
        bool takePayload(const ResourceHandle<T> handle, T& out) {
            T* payload = static_cast<T*>(resolvePayload(handle.slot(), resourceTypeTag<T>()));
            if (payload == nullptr) {
                return false;
            }
            out = std::move(*payload);
            *payload = T{};
            return true;
        }

        void submitLoads();

        std::error_code removeCompletions();

        std::error_code runLoadingPhase();

        void drainFrees();

        void shutdown();

    private:
        template<typename T>
        static void* createResourcePayload() {
            return new T{};
        }

        template<typename T>
        static void destroyResourcePayload(void* payload) {
            delete static_cast<T*>(payload);
        }

        void registerType(const BlobKind& kind, StringId typeTag, StringId existingTypeTag, ResourceLoadFn load, void* (*createPayload)(), void (*destroyPayload)(void*));

        std::error_code requestEntry(ResourceId id, StringId typeTag, ResourceSlotId& out);

        void releaseEntry(ResourceSlotId slot);

        const void* resolvePayload(ResourceSlotId slot, StringId typeTag) const;

        void* resolvePayload(ResourceSlotId slot, StringId typeTag);

        std::error_code addEntryDependency(ResourceSlotId holder, ResourceId dependency, ResourceSlotId& out);

        bool entryState(ResourceSlotId slot, ResourceState& out) const;

        bool entryAcquireCount(ResourceSlotId slot, std::uint32_t& out) const;

        bool entryError(ResourceSlotId slot, std::error_code& out) const;

        // Runs on a pool worker, so it may touch only `entry` and the completion queue. The
        // owner grows m_slots while tasks run, and looking any entry up from here races it.
        void runLoadTask(ResourceEntry& entry, ResourceSlotId slot);

        std::error_code publishCompletion(const LoadCompletion& completion);

        void freeEntry(ResourceSlotId slot);

        bool reaches(ResourceSlotId from, ResourceSlotId target) const;

        const TypeRecord* findTypeForBytes(std::span<const std::byte> bytes) const;
    };
}
