#pragma once

#include "event/events/Event.hpp"

namespace TechEngine {
    class AppCloseRequestEvent : public TechEngineCore::Event {
    public:
        static inline EventType eventType = EventType("AppCloseRequestEvent", ASYNC);

        AppCloseRequestEvent() : Event(eventType) {
        };

        ~AppCloseRequestEvent() override = default;
    };
}
