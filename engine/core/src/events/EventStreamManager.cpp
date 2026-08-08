#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/events/EventStreamManager.hpp>

namespace TechEngine {
    EventStreamManager::EventStreamManager(EventRegistry& registry, const std::size_t initialCapacity) : m_registry{&registry} {
        registry.seal();
        m_streams.reserve(registry.typeCount());

        for (const EventTypeRecord& record: registry.records()) {
            m_streams.emplace_back(record.id, record.size, record.alignment, initialCapacity);
        }
    }

    void EventStreamManager::makeVisible(const std::uint64_t frameIndex, const std::uint64_t tick) {
        TE_PROFILER_SCOPE("EventStreamManager.MakeVisible");

        for (EventStream& stream: m_streams) {
            stream.makeVisible(frameIndex, tick);
        }
    }

    void EventStreamManager::retire(const std::uint64_t frameIndex, const std::uint64_t tick) {
        TE_PROFILER_SCOPE("EventStreamManager.Retire");

        for (EventStream& stream: m_streams) {
            stream.retire(frameIndex, tick);
        }
    }

    std::size_t EventStreamManager::streamCount() const {
        return m_streams.size();
    }

    EventStream* EventStreamManager::getStream(const EventTypeId id) {
        const EventTypeRecord* record = m_registry->find(id);
        if (!TE_VERIFY(record != nullptr && record->streamIndex < m_streams.size(), "Event type {0} has no stream", id.stringId().value())) {
            return nullptr;
        }
        return &m_streams[record->streamIndex];
    }

    const EventStream* EventStreamManager::getStream(const EventTypeId id) const {
        const EventTypeRecord* record = m_registry->find(id);
        if (!TE_VERIFY(record != nullptr && record->streamIndex < m_streams.size(), "Event type {0} has no stream", id.stringId().value())) {
            return nullptr;
        }
        return &m_streams[record->streamIndex];
    }
}
