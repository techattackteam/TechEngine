#pragma once

#include "core/Core.hpp"

namespace TechEngine {
    class WindowCloseEvent : public TechEngineCore::Event {
    public:
        inline static EventType eventType = EventType("WindowCloseEvent", SYNC);

        explicit WindowCloseEvent() : Event(eventType) {
        };

        ~WindowCloseEvent() override = default;
    };
}

