#pragma once

#include "EventType.hpp"

namespace TechEngineCore {
    class Event {
    protected:
        EventType eventType;

        explicit Event(EventType eventType);

    public:
        virtual ~Event();

        EventType getEventType();
    };
}

