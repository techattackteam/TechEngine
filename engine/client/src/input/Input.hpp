#pragma once

#include <vector>

#include "Key.hpp"
#include "Mouse.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class CLIENT_DLL Input : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        Mouse mouse;
        bool m_editor;
        std::vector<bool> m_keyStates = std::vector<bool>(350, false); // Assuming a max of 350 key codes
        bool m_isCapsLockActive = false;

    public:
        Input(SystemsRegistry& systemsRegistry);

        void init();

        void onKeyPressed(KeyCode key);

        void onKeyReleased(KeyCode key);

        void onKeyHold(KeyCode key);

        void onMouseMove(double xpos, double ypos);

        void onMouseScroll(double xoffset, double yoffset);

        bool isKeyPressed(KeyCode key);

        bool isCapsLockActive();

        Mouse& getMouse();
    };
}
