#include <TechEngine/core/resources/ResourceCache.hpp>

#include <resources/ResourceSlots.hpp>

#include <memory>

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

    void ResourceCache::registerType(const BlobKind&, StringId, StringId, ResourceLoadFn, void* (*)(), void (*)(void*)) {
        // TODO(S8-T10): check that registration is still open, that the tag and the magic are
        // each unregistered, that the tag is not null, and that this type was not already
        // registered under another tag; then append the record.
    }

    std::error_code ResourceCache::requestEntry(ResourceId, StringId, ResourceSlotId&) {
        // TODO(S8-T10): an id already in m_byId adds one to its count and hands back its slot.
        // Otherwise ask the source: Unknown fails at once with ReadError::UnknownResource and
        // creates nothing, AbsentForRole creates an entry in that state, and Present creates a
        // pending entry the next loading phase reads. The requested type tag is kept on the
        // entry so the bytes' own tag can be checked against it.
        return {};
    }

    void ResourceCache::releaseEntry(ResourceSlotId) {
        // TODO(S8-T10): a stale slot releases nothing. Take one off the count and, at zero,
        // push the slot onto m_pendingFrees; nothing is freed here.
    }

    const void* ResourceCache::resolvePayload(ResourceSlotId, StringId) const {
        // TODO(S8-T10): nullptr for a stale slot or an entry that is not Ready. A tag that
        // does not match the entry's is a TE_CHECK: the handle was resolved as the wrong type.
        return nullptr;
    }

    bool ResourceCache::entryState(ResourceSlotId, ResourceState&) const {
        // TODO(S8-T10): false for a stale slot, otherwise fill the entry's state.
        return false;
    }

    bool ResourceCache::entryAcquireCount(ResourceSlotId, std::uint32_t&) const {
        // TODO(S8-T10): false for a stale slot, otherwise fill the entry's acquire count.
        return false;
    }

    std::error_code ResourceCache::runLoadingPhase() {
        // TODO(S8-T10): until nothing is pending, read each pending entry's bytes, pick its
        // loader by the leading magic, run it, publish the entry and request its dependencies
        // as the next round, acquiring one on each. A loader's error marks the entry Failed
        // and ends the phase with that error unchanged. Drain the frees before returning.
        return {};
    }

    void ResourceCache::drainFrees() {
        // TODO(S8-T10): for each pending free still at zero, release its dependencies, destroy
        // its payload, drop it from m_byId and give the slot back. An entry that was acquired
        // again since it was queued stays.
    }
}
