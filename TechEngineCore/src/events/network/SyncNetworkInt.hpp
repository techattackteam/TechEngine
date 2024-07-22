#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class SyncNetworkInt : public Event {
    private:
        int value;

    public:
        inline static EventType eventType = EventType("SyncNetworkInt", SYNC);

        explicit SyncNetworkInt(int value) : value(value), Event(eventType) {
        }

        ~SyncNetworkInt() override = default;

        int getValue() const {
            return value;
        }
    };
}
