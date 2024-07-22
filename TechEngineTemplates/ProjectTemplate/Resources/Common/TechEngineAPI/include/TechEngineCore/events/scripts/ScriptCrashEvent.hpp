#pragma once
#include "events/Event.hpp"

namespace TechEngine {
    class ScriptCrashEvent : public Event {
    public:
        inline static EventType eventType = EventType("ScriptCrashEvent", ASYNC);

        explicit ScriptCrashEvent() : Event(eventType) {
        };

        ~ScriptCrashEvent() override = default;
    };
}
