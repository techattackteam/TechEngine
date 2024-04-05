#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class MouseMoveEvent : public Event {
    private:
        glm::vec2 fromPosition;
        glm::vec2 toPosition;
        glm::vec2 delta;
    public:

        static inline EventType eventType = EventType("MouseMoveEvent", EventTiming::ASYNC);

        MouseMoveEvent(glm::vec2 fromPosition, glm::vec2 toPosition) : Event(eventType) {
            this->fromPosition = fromPosition;
            this->toPosition = toPosition;
            this->delta = toPosition - fromPosition;
        };

        ~MouseMoveEvent() override = default;

        glm::vec2 &getFromPosition() {
            return fromPosition;
        }

        glm::vec2 &getToPosition() {
            return toPosition;
        }

        glm::vec2 &getDelta() {
            return delta;
        }

    };
}
