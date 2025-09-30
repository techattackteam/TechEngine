#pragma once

#include "../../input/Key.hpp"
#include "TechEngine/core/events/Event.hpp"

namespace TechEngine {
    class KeyHoldEvent : public Event {
    private:
        Key key;

    public:

        explicit KeyHoldEvent(Key key) : key(key) {
        };

        Key& getKey() {
            return key;
        };
    };
}