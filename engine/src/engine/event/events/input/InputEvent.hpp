#pragma once


#include "../Event.hpp"

namespace Engine {
    class InputEvent : public Event {

    public:
        inline static EventType eventType = EventType("InputEvent", SYNC);

        InputEvent() : Event(eventType) {

        }
    };
}


