#pragma once

#include "events/Event.hpp"
#include <glm/vec2.hpp>


namespace TechEngine {
    class MouseMoveEvent : public Event {
    private:
        glm::vec2 fromPosition;
        glm::vec2 toPosition;
        glm::vec2 delta;

    public:
        MouseMoveEvent(glm::vec2 fromPosition, glm::vec2 toPosition) : fromPosition(fromPosition),
                                                                       toPosition(toPosition),
                                                                       delta(toPosition - fromPosition) {
        };

        ~MouseMoveEvent() override = default;

        glm::vec2& getFromPosition() {
            return fromPosition;
        }

        glm::vec2& getToPosition() {
            return toPosition;
        }

        glm::vec2& getDelta() {
            return delta;
        }
    };
}
