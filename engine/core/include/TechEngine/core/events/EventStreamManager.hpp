#pragma once

#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/events/EventStream.hpp>
#include <TechEngine/core/events/EventTypeId.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace TechEngine {
    class EventStreamManager {
    private:
        const EventRegistry* m_registry{nullptr};
        std::vector<EventStream> m_streams;

    public:
        explicit EventStreamManager(EventRegistry& registry, std::size_t initialCapacity = 64);

        EventStreamManager(const EventStreamManager&) = delete;

        EventStreamManager& operator=(const EventStreamManager&) = delete;

        EventStreamManager(EventStreamManager&&) noexcept = default;

        EventStreamManager& operator=(EventStreamManager&&) noexcept = default;

        template<typename T>
        void publish(const T& event) {
            EventStream* stream = getStream(eventTypeId<T>());
            if (stream == nullptr) {
                return;
            }
            stream->publish(event);
        }

        template<typename T>
        std::span<const T> read(EventCursor& cursor) const {
            const EventStream* stream = getStream(eventTypeId<T>());
            if (stream == nullptr) {
                return {};
            }
            return stream->read<T>(cursor);
        }

        void makeVisible(std::uint64_t frameIndex, std::uint64_t tick);

        void retire(std::uint64_t frameIndex, std::uint64_t tick);

        std::size_t streamCount() const;

    private:
        EventStream* getStream(EventTypeId id);

        const EventStream* getStream(EventTypeId id) const;
    };
}
