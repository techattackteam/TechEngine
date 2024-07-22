#pragma once

#include "EventType.hpp"
#include "core/CoreExportDll.hpp"

namespace TechEngine {
    class CORE_DLL Event {
    protected:
        EventType eventType;

        explicit Event(EventType eventType);

    public:
        virtual ~Event();

        EventType getEventType();
    };
}
