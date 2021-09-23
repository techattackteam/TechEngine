#pragma once

#include "../Event.hpp"
#include "../../../core/Key.hpp"

namespace Engine {
    class KeyReleasedEvent : public Event {
    private:
        Key key;
    public:
        static inline EventType eventType = EventType("KeyReleasedEvent", EventTiming::ASYNC);


        KeyReleasedEvent(Key key) : key(key), Event(eventType) {

        };

        ~KeyReleasedEvent() override = default;

        Key &getKey() {
            return key;
        };
    };
}
