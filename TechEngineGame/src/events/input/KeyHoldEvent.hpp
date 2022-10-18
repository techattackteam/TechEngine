#pragma once

#include "core/Key.hpp"
#include "core/Core.hpp"
namespace TechEngine {
    class Engine_API KeyHoldEvent : public TechEngineCore::Event {
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
