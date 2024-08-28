#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class MouseScrollEvent : public Event {
    private:
        float xOffset;
        float yOffset;
    public:

        MouseScrollEvent(float xOffset, float yOffset) : xOffset(xOffset), yOffset(yOffset) {

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