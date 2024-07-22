#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class NetworkIntValueChanged : public Event {
    private:
        int value;

    public:
        inline static EventType eventType = EventType("NetworkIntValueChanged", SYNC);

        explicit NetworkIntValueChanged(int value) : value(value), Event(eventType) {
        }

        ~NetworkIntValueChanged() override = default;

        int getValue() const {
            return value;
        }
    };
}
