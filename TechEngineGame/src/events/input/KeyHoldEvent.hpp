#pragma once

#include <TechEngineCore.hpp>
#include "../../core/Key.hpp"

namespace TechEngine {
    class KeyHoldEvent : public TechEngineCore::Event {
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
