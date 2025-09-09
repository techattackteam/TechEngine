#pragma once

#include "Key.hpp"
#include "Mouse.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class CLIENT_DLL Input : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        Mouse mouse;
        bool m_editor;

    public:
        Input(SystemsRegistry& systemsRegistry);

        void init();

        void onKeyInput(KeyCode key, int action);

        void onMouseMove(double xpos, double ypos);

        void onMouseScroll(double xoffset, double yoffset);

        Mouse& getMouse();
    };
}
