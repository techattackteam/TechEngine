#pragma once

#include "event/events/Event.hpp"

namespace TechEngine {

    class MouseScrollEvent : public Event {
    private:
        float xOffset;
        float yOffset;
    public:

        static inline EventType eventType = EventType("MouseScrollEvent", EventTiming::ASYNC);

        MouseScrollEvent(float xOffset, float yOffset) : xOffset(xOffset), yOffset(yOffset), Event(eventType) {

        };

        ~MouseScrollEvent() override = default;

        float getXOffset() {
            return xOffset;
        };

        float getYOffset() {
            return yOffset;
        };

    };
}
