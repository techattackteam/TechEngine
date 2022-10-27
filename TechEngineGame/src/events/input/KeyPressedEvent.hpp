#pragma once

#include "../../../../TechEngineCore/src/event/events/Event.hpp"
#include "../../core/Key.hpp"
#include "core/Core.hpp"
namespace TechEngine {
class /*Engine_API*/ KeyPressedEvent : public TechEngineCore::Event {
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
