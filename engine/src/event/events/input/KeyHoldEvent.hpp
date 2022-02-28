#pragma once

#include "../Event.hpp"
#include "../../../core/Key.hpp"

namespace Engine {
    class KeyHoldEvent : public Event {
    private:
        Key key;
    public:
        inline static EventType eventType = EventType("KeyHoldEvent", EventTiming::ASYNC);

        KeyHoldEvent(Key key) : key(key), Event(eventType) {

        };

        Key &getKey() {
            return key;
        };
    };
}
