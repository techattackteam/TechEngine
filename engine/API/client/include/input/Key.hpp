#pragma once

#include "common/include/core/ExportDLL.hpp"
#include <string>

namespace TechEngineAPI {
    enum KeyCode : unsigned long long {
        None = 0,
        Q = 1ULL << 0,
        W = 1ULL << 1,
        E = 1ULL << 2,
        R = 1ULL << 3,
        T = 1ULL << 4,
        Y = 1ULL << 5,
        U = 1ULL << 6,
        I = 1ULL << 7,
        O = 1ULL << 8,
        P = 1ULL << 9,
        A = 1ULL << 10,
        S = 1ULL << 11,
        D = 1ULL << 12,
        F = 1ULL << 13,
        G = 1ULL << 14,
        H = 1ULL << 15,
        J = 1ULL << 16,
        K = 1ULL << 17,
        L = 1ULL << 18,
        Z = 1ULL << 19,
        X = 1ULL << 20,
        C = 1ULL << 21,
        V = 1ULL << 22,
        B = 1ULL << 23,
        N = 1ULL << 24,
        M = 1ULL << 25,
        ONE = 1ULL << 26,
        TWO = 1ULL << 27,
        THREE = 1ULL << 28,
        FOUR = 1ULL << 29,
        FIVE = 1ULL << 30,
        SIX = 1ULL << 31,
        SEVEN = 1ULL << 32,
        EIGHT = 1ULL << 33,
        NINE = 1ULL << 34,
        ZERO = 1ULL << 35,
        ARROW_UP = 1ULL << 36,
        ARROW_DOWN = 1ULL << 37,
        ARROW_RIGHT = 1ULL << 38,
        ARROW_LEFT = 1ULL << 39,
        SPACE = 1ULL << 40,
        ENTER = 1ULL << 41,
        TAB = 1ULL << 42,
        BACKSPACE = 1ULL << 43,
        DEL = 1ULL << 44,
        LEFT_SHIFT = 1ULL << 45,
        RIGHT_SHIFT = 1ULL << 46,
        LEFT_CTRL = 1ULL << 47,
        RIGHT_CTRL = 1ULL << 48,
        LEFT_ALT = 1ULL << 49,
        RIGHT_ALT = 1ULL << 50,
        MOUSE_1 = 1ULL << 51,
        MOUSE_2 = 1ULL << 52,
        MOUSE_3 = 1ULL << 53,
        MOUSE_4 = 1ULL << 54,
        MOUSE_5 = 1ULL << 55,
        MOUSE_6 = 1ULL << 56,
        MOUSE_7 = 1ULL << 57,
        MOUSE_8 = 1ULL << 58,
        ESC = 1ULL << 59
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
