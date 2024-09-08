#pragma once

#include "events/Event.hpp"
#include "input/Key.hpp"

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
