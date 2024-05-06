#pragma once
#include "eventSystem/EventDispatcher.hpp"
#include <glm/vec2.hpp>

namespace TechEngine {
    class Mouse {
    private:
        EventDispatcher& eventDispatcher;
        bool button_pressed[8] = {false, false, false, false, false, false, false, false};

        glm::vec2 lastPosition = glm::vec2(0, 0);

    public:
        Mouse(EventDispatcher& eventDispatcher);

        void onUpdate();

        void onMouseMove(double x, double y);

        Mouse(const Mouse&) = delete;

        Mouse& operator=(const Mouse&) = delete;
    };
}
