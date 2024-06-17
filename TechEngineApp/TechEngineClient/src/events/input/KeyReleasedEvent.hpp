#pragma once

#include "core/Key.hpp"

namespace TechEngine {
    class KeyReleasedEvent : public TechEngine::Event {
    private:
        Key key;

    public:
        static inline EventType eventType = EventType("KeyReleasedEvent", EventTiming::ASYNC);

        KeyReleasedEvent(Key key) : key(key), Event(eventType) {
        };

        ~KeyReleasedEvent() override = default;

        Key& getKey() {
            return key;
        };
    };
}
