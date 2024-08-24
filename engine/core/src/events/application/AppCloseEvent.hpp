#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class CORE_DLL AppCloseEvent : public Event {
    public:
        AppCloseEvent() = default;
    };
}
