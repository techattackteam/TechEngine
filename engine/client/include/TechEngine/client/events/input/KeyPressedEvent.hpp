#pragma once

#include "TechEngine/core/events/Event.hpp"
#include "TechEngine/client/input/Key.hpp"

namespace TechEngine {
    class KeyPressedEvent : public Event {
    private:
        Key key;

    public:
        explicit KeyPressedEvent(Key key) : key(key) {
        };

        ~KeyPressedEvent() override = default;

        Key& getKey() {
            return key;
        };
    };
}
