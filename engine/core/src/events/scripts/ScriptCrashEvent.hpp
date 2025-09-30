#pragma once
#include "TechEngine/core/events/Event.hpp"

namespace TechEngine {
    class ScriptCrashEvent : public Event {
    public:
        ScriptCrashEvent() = default;
    };
}
