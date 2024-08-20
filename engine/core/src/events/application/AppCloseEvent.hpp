#pragma once

#include "core/ExportDLL.hpp"
#include "events/Event.hpp"

namespace TechEngine {
    class CORE_DLL AppCloseEvent : public Event {
    public:
        AppCloseEvent() = default;
    };
}
