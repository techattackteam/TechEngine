#pragma once

#include <events/Event.hpp>

namespace TechEngine {
    class OnClientConnectionRequest : public Event {
    public:
        inline static EventType eventType = EventType("OnClientConnectionRequest", EventTiming::SYNC);

        OnClientConnectionRequest() : Event(eventType) {
        };

        ~OnClientConnectionRequest() override = default;
    };
}
