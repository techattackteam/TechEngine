#pragma once

#include "../Event.hpp"
#include "../../../core/Key.hpp"

namespace Engine {
    class KeyPressedEvent : public Event {
    private:
        Key key;
    public:
        static inline EventType eventType = EventType("KeyPressedEvent", EventTiming::ASYNC);


        KeyPressedEvent(Key key) : key(key), Event(eventType) {

        };

        ~KeyPressedEvent() override = default;

        Key &getKey() {
            return key;
        };
    };
}
