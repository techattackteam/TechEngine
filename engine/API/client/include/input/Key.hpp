#pragma once

#include "common/include/core/ExportDLL.hpp"
#include <string>

namespace TechEngineAPI {
    enum KeyCode {
        None, Q, W, E, R, T, Y, U, I, O, P, A, S, D, F, G, H, J, K, L, Z, X, C, V, B, N, M,
        ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, ZERO,
        ARROW_UP, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT,
        SPACE, ENTER, TAB, BACKSPACE, DEL,
        LEFT_SHIFT, RIGHT_SHIFT,
        LEFT_CTRL, RIGHT_CTRL,
        LEFT_ALT, RIGHT_ALT,
        MOUSE_1, MOUSE_2, MOUSE_3, MOUSE_4, MOUSE_5, MOUSE_6, MOUSE_7, MOUSE_8,
        ESC
    };

    class API_DLL Key {
    private:
        KeyCode keyCode;

    public:
        explicit Key(KeyCode keyCode);

        KeyCode& getKeyCode();

        std::string getKeyName();
    };
}
