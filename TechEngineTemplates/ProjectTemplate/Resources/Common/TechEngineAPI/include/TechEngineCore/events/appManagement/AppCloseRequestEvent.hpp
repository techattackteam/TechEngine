#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class AppCloseRequestEvent : public TechEngine::Event {
    public:
        static inline EventType eventType = EventType("AppCloseRequestEvent", SYNC);

        AppCloseRequestEvent() : Event(eventType) {
        };

        ~AppCloseRequestEvent() override = default;
    };
}
