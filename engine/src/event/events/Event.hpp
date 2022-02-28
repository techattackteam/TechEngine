#pragma once

#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include "EventType.hpp"

namespace Engine {
    class Event {
    protected:
        EventType eventType;

        explicit Event(EventType eventType);

    public:
        virtual ~Event();

        EventType getEventType();
    };
}

