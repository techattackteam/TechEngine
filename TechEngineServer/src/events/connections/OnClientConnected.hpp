#pragma once

#include <events/Event.hpp>

namespace TechEngine {
    class OnClientConnected : public Event {
    public:
        inline static EventType eventType = EventType("OnClientConnected", EventTiming::SYNC);

        OnClientConnected() : Event(eventType) {
        };

        ~OnClientConnected() override = default;
    };
}
