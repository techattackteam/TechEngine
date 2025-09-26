#pragma once

#include "input/Key.hpp"
#include "events/Event.hpp"

namespace TechEngine {
    class KeyReleasedEvent : public Event {
    private:
        Key key;

    public:
        KeyReleasedEvent(Key key) : key(key) {
        };

        ~KeyReleasedEvent() override = default;

        Key& getKey() {
            return key;
        };
    };
}
