#pragma once
#include "events/Event.hpp"

namespace TechEngine {
    class ScriptCrashEvent : public Event {
    public:
        ScriptCrashEvent() = default;
    };
}
