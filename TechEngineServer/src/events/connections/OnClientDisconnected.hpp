#pragma once

#include <events/Event.hpp>

namespace TechEngine {
    class OnClientDisconnected : public Event {
    public:
        inline static EventType eventType = EventType("OnClientDisconnected", EventTiming::SYNC);

        OnClientDisconnected() : Event(eventType) {
        };

        ~OnClientDisconnected() override = default;
    };
}
