#pragma once

#include "core/Key.hpp"
#include "event/events/Event.hpp"

namespace TechEngine {
    class KeyHoldEvent : public Event {
    private:
        Key key;

    public:
        inline static EventType eventType = EventType("KeyHoldEvent", EventTiming::ASYNC);

        KeyHoldEvent(Key key) : key(key), Event(eventType) {
        };

        Key& getKey() {
            return key;
        };
    };
}
