#pragma once

#include "event/events/Event.hpp"

namespace TechEngine {
    class WindowCloseEvent : public TechEngine::Event {
    public:
        inline static EventType eventType = EventType("WindowCloseEvent", SYNC);

        explicit WindowCloseEvent() : Event(eventType) {
        };

        ~WindowCloseEvent() override = default;
    };
}

