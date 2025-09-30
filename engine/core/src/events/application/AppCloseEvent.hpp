#pragma once

#include "TechEngine/core/events/Event.hpp"

namespace TechEngine {
    class CORE_DLL AppCloseEvent : public Event {
    public:
        AppCloseEvent() = default;
    };
}
