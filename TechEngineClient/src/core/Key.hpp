#pragma once

#include <string>
#include <GLFW/glfw3.h>

enum KeyCode {
    None, Q, W, E, R, T, Y, U, I, O, P, A, S, D, F, G, H, J, K, L, Z, X, C, V, B, N, M,
    ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, ZERO,
    ARROW_UP, ARROW_DOWN, ARROW_RIGHT, ARROW_LEFT,
    ESC
};

class Key {
public:
    explicit Key(KeyCode keyCode) : keyCode(keyCode) {
    }

    KeyCode &getKeyCode();

    std::string getKeyName();

    static KeyCode getKeyCode(int key);

private:
    KeyCode keyCode;
};

