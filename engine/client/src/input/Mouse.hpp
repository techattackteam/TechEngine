#pragma once

#include "core/ExportDll.hpp"
#include <glm/vec2.hpp>

namespace TechEngine {
    class SystemsRegistry;

    class CLIENT_DLL Mouse {
    private:
        SystemsRegistry& m_systemsRegistry;
        bool m_buttonsPressed[8] = {false, false, false, false, false, false, false, false};

        glm::vec2 m_lastPosition = glm::vec2(0, 0);

    public:
        Mouse(SystemsRegistry& systemsRegistry);

        void onUpdate();

        void onMouseMove(double x, double y);

        Mouse(const Mouse&) = delete;

        Mouse& operator=(const Mouse&) = delete;
    };
}
