#pragma once

#include "core/Key.hpp"
#include "core/Core.hpp"

namespace TechEngine {
    class /*Engine_API*/ KeyReleasedEvent : public TechEngineCore::Event {
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
