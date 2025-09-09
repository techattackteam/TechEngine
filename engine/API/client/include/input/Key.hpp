#pragma once

#include "common/include/core/ExportDLL.hpp"
#include <string>

namespace TechEngineAPI {
    enum KeyCode {
        None,

        // Letters
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Numbers (Top Row)
        ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,

        // Function Keys
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

        // Arrow Keys
        ARROW_UP, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT,

        // Special & Formatting
        ESC,
        SPACE,
        ENTER,
        TAB,
        BACKSPACE,
        DEL, // Delete
        INSERT,
        HOME,
        END,
        PAGE_UP,
        PAGE_DOWN,

        // Modifier Keys
        LEFT_SHIFT, RIGHT_SHIFT,
        LEFT_CTRL, RIGHT_CTRL,
        LEFT_ALT, RIGHT_ALT,
        CAPS_LOCK,

        // Punctuation & Symbols
        GRAVE_ACCENT, // `
        MINUS, // -
        EQUAL, // =
        LEFT_BRACKET, // [
        RIGHT_BRACKET, // ]
        BACKSLASH, // '\;
            SEMICOLON,      // ;
        APOSTROPHE, // '
        COMMA, // ,
        PERIOD, // .
        SLASH, // /

        // Numpad Keys
        KP_0, KP_1, KP_2, KP_3, KP_4, KP_5, KP_6, KP_7, KP_8, KP_9,
        KP_DECIMAL,
        KP_DIVIDE,
        KP_MULTIPLY,
        KP_SUBTRACT,
        KP_ADD,
        KP_ENTER,

        // Mouse Buttons
        MOUSE_1, MOUSE_2, MOUSE_3, MOUSE_4, MOUSE_5,
        MOUSE_6, MOUSE_7, MOUSE_8,
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
