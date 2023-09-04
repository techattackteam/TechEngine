#pragma once

namespace TechEngine {
    class Mouse {
    private:
        bool button_pressed[8] = {false, false, false, false, false, false, false, false};

        inline static Mouse *instance;
        glm::vec2 lastPosition = glm::vec2(0, 0);
    public:
        Mouse();

        void onUpdate();

        void onMouseMove(double x, double y);

        static Mouse &getInstance();
    };

}
