#pragma once
#include "eventSystem/EventDispatcher.hpp"
#include "core/ClientExportDll.hpp"
#include <glm/vec2.hpp>

namespace TechEngine {
    class CLIENT_DLL Mouse {
    private:
        SystemsRegistry& systemsRegistry;
        bool button_pressed[8] = {false, false, false, false, false, false, false, false};

        glm::vec2 lastPosition = glm::vec2(0, 0);

    public:
        Mouse(SystemsRegistry& systemsRegistry);

        void onUpdate();

        void onMouseMove(double x, double y);

        Mouse(const Mouse&) = delete;

        Mouse& operator=(const Mouse&) = delete;
    };
}
