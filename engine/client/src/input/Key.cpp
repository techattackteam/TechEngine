#include "Key.hpp"
#include <GLFW/glfw3.h>

namespace TechEngine {
    KeyCode& Key::getKeyCode() {
        return keyCode;
    }


    std::string Key::getKeyName() const {
        switch (keyCode) {
            // Letters
            case A: return "A";
            case B: return "B";
            case C: return "C";
            case D: return "D";
            case E: return "E";
            case F: return "F";
            case G: return "G";
            case H: return "H";
            case I: return "I";
            case J: return "J";
            case K: return "K";
            case L: return "L";
            case M: return "M";
            case N: return "N";
            case O: return "O";
            case P: return "P";
            case Q: return "Q";
            case R: return "R";
            case S: return "S";
            case T: return "T";
            case U: return "U";
            case V: return "V";
            case W: return "W";
            case X: return "X";
            case Y: return "Y";
            case Z: return "Z";

            // Numbers
            case ZERO: return "0";
            case ONE: return "1";
            case TWO: return "2";
            case THREE: return "3";
            case FOUR: return "4";
            case FIVE: return "5";
            case SIX: return "6";
            case SEVEN: return "7";
            case EIGHT: return "8";
            case NINE: return "9";

            // Function Keys
            case F1: return "F1";
            case F2: return "F2";
            case F3: return "F3";
            case F4: return "F4";
            case F5: return "F5";
            case F6: return "F6";
            case F7: return "F7";
            case F8: return "F8";
            case F9: return "F9";
            case F10: return "F10";
            case F11: return "F11";
            case F12: return "F12";

            // Arrow Keys
            case ARROW_UP: return "UP";
            case ARROW_DOWN: return "DOWN";
            case ARROW_RIGHT: return "RIGHT";
            case ARROW_LEFT: return "LEFT";

            // Special & Formatting
            case ESC: return "Escape";
            case SPACE: return "Space";
            case ENTER: return "Enter";
            case TAB: return "Tab";
            case BACKSPACE: return "Backspace";
            case DEL: return "Delete";
            case INSERT: return "Insert";
            case HOME: return "Home";
            case END: return "End";
            case PAGE_UP: return "Page Up";
            case PAGE_DOWN: return "Page Down";

            // Modifier Keys
            case LEFT_SHIFT: return "Left Shift";
            case RIGHT_SHIFT: return "Right Shift";
            case LEFT_CTRL: return "Left Ctrl";
            case RIGHT_CTRL: return "Right Ctrl";
            case LEFT_ALT: return "Left Alt";
            case RIGHT_ALT: return "Right Alt";
            case CAPS_LOCK: return "Caps Lock";

            // Punctuation & Symbols
            case GRAVE_ACCENT: return "`";
            case MINUS: return "-";
            case EQUAL: return "=";
            case LEFT_BRACKET: return "[";
            case RIGHT_BRACKET: return "]";
            case BACKSLASH: return "\\";
            case SEMICOLON: return ";";
            case APOSTROPHE: return "'";
            case COMMA: return ",";
            case PERIOD: return ".";
            case SLASH: return "/";

            // Numpad Keys
            case KP_0: return "Numpad 0";
            case KP_1: return "Numpad 1";
            case KP_2: return "Numpad 2";
            case KP_3: return "Numpad 3";
            case KP_4: return "Numpad 4";
            case KP_5: return "Numpad 5";
            case KP_6: return "Numpad 6";
            case KP_7: return "Numpad 7";
            case KP_8: return "Numpad 8";
            case KP_9: return "Numpad 9";
            case KP_DECIMAL: return "Numpad .";
            case KP_DIVIDE: return "Numpad /";
            case KP_MULTIPLY: return "Numpad *";
            case KP_SUBTRACT: return "Numpad -";
            case KP_ADD: return "Numpad +";
            case KP_ENTER: return "Numpad Enter";

            // Mouse Buttons
            case MOUSE_1: return "Mouse 1";
            case MOUSE_2: return "Mouse 2";
            case MOUSE_3: return "Mouse 3";
            case MOUSE_4: return "Mouse 4";
            case MOUSE_5: return "Mouse 5";
            case MOUSE_6: return "Mouse 6";
            case MOUSE_7: return "Mouse 7";
            case MOUSE_8: return "Mouse 8";

            default:
                return "";
        }
    }

    KeyCode Key::getKeyCode(int key) const {
        switch (key) {
            // Letters
            case GLFW_KEY_A: return KeyCode::A;
            case GLFW_KEY_B: return KeyCode::B;
            case GLFW_KEY_C: return KeyCode::C;
            case GLFW_KEY_D: return KeyCode::D;
            case GLFW_KEY_E: return KeyCode::E;
            case GLFW_KEY_F: return KeyCode::F;
            case GLFW_KEY_G: return KeyCode::G;
            case GLFW_KEY_H: return KeyCode::H;
            case GLFW_KEY_I: return KeyCode::I;
            case GLFW_KEY_J: return KeyCode::J;
            case GLFW_KEY_K: return KeyCode::K;
            case GLFW_KEY_L: return KeyCode::L;
            case GLFW_KEY_M: return KeyCode::M;
            case GLFW_KEY_N: return KeyCode::N;
            case GLFW_KEY_O: return KeyCode::O;
            case GLFW_KEY_P: return KeyCode::P;
            case GLFW_KEY_Q: return KeyCode::Q;
            case GLFW_KEY_R: return KeyCode::R;
            case GLFW_KEY_S: return KeyCode::S;
            case GLFW_KEY_T: return KeyCode::T;
            case GLFW_KEY_U: return KeyCode::U;
            case GLFW_KEY_V: return KeyCode::V;
            case GLFW_KEY_W: return KeyCode::W;
            case GLFW_KEY_X: return KeyCode::X;
            case GLFW_KEY_Y: return KeyCode::Y;
            case GLFW_KEY_Z: return KeyCode::Z;

            // Numbers (Top Row)
            case GLFW_KEY_0: return KeyCode::ZERO;
            case GLFW_KEY_1: return KeyCode::ONE;
            case GLFW_KEY_2: return KeyCode::TWO;
            case GLFW_KEY_3: return KeyCode::THREE;
            case GLFW_KEY_4: return KeyCode::FOUR;
            case GLFW_KEY_5: return KeyCode::FIVE;
            case GLFW_KEY_6: return KeyCode::SIX;
            case GLFW_KEY_7: return KeyCode::SEVEN;
            case GLFW_KEY_8: return KeyCode::EIGHT;
            case GLFW_KEY_9: return KeyCode::NINE;

            // Function Keys
            case GLFW_KEY_F1: return KeyCode::F1;
            case GLFW_KEY_F2: return KeyCode::F2;
            case GLFW_KEY_F3: return KeyCode::F3;
            case GLFW_KEY_F4: return KeyCode::F4;
            case GLFW_KEY_F5: return KeyCode::F5;
            case GLFW_KEY_F6: return KeyCode::F6;
            case GLFW_KEY_F7: return KeyCode::F7;
            case GLFW_KEY_F8: return KeyCode::F8;
            case GLFW_KEY_F9: return KeyCode::F9;
            case GLFW_KEY_F10: return KeyCode::F10;
            case GLFW_KEY_F11: return KeyCode::F11;
            case GLFW_KEY_F12: return KeyCode::F12;

            // Arrow Keys
            case GLFW_KEY_UP: return KeyCode::ARROW_UP;
            case GLFW_KEY_DOWN: return KeyCode::ARROW_DOWN;
            case GLFW_KEY_RIGHT: return KeyCode::ARROW_RIGHT;
            case GLFW_KEY_LEFT: return KeyCode::ARROW_LEFT;

            // Special & Formatting
            case GLFW_KEY_ESCAPE: return KeyCode::ESC;
            case GLFW_KEY_SPACE: return KeyCode::SPACE;
            case GLFW_KEY_ENTER: return KeyCode::ENTER;
            case GLFW_KEY_TAB: return KeyCode::TAB;
            case GLFW_KEY_BACKSPACE: return KeyCode::BACKSPACE;
            case GLFW_KEY_DELETE: return KeyCode::DEL;
            case GLFW_KEY_INSERT: return KeyCode::INSERT;
            case GLFW_KEY_HOME: return KeyCode::HOME;
            case GLFW_KEY_END: return KeyCode::END;
            case GLFW_KEY_PAGE_UP: return KeyCode::PAGE_UP;
            case GLFW_KEY_PAGE_DOWN: return KeyCode::PAGE_DOWN;

            // Modifier Keys
            case GLFW_KEY_LEFT_SHIFT: return KeyCode::LEFT_SHIFT;
            case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RIGHT_SHIFT;
            case GLFW_KEY_LEFT_CONTROL: return KeyCode::LEFT_CTRL;
            case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RIGHT_CTRL;
            case GLFW_KEY_LEFT_ALT: return KeyCode::LEFT_ALT;
            case GLFW_KEY_RIGHT_ALT: return KeyCode::RIGHT_ALT;
            case GLFW_KEY_CAPS_LOCK: return KeyCode::CAPS_LOCK;

            // Punctuation & Symbols
            case GLFW_KEY_GRAVE_ACCENT: return KeyCode::GRAVE_ACCENT;
            case GLFW_KEY_MINUS: return KeyCode::MINUS;
            case GLFW_KEY_EQUAL: return KeyCode::EQUAL;
            case GLFW_KEY_LEFT_BRACKET: return KeyCode::LEFT_BRACKET;
            case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RIGHT_BRACKET;
            case GLFW_KEY_BACKSLASH: return KeyCode::BACKSLASH;
            case GLFW_KEY_SEMICOLON: return KeyCode::SEMICOLON;
            case GLFW_KEY_APOSTROPHE: return KeyCode::APOSTROPHE;
            case GLFW_KEY_COMMA: return KeyCode::COMMA;
            case GLFW_KEY_PERIOD: return KeyCode::PERIOD;
            case GLFW_KEY_SLASH: return KeyCode::SLASH;

            // Numpad Keys
            case GLFW_KEY_KP_0: return KeyCode::KP_0;
            case GLFW_KEY_KP_1: return KeyCode::KP_1;
            case GLFW_KEY_KP_2: return KeyCode::KP_2;
            case GLFW_KEY_KP_3: return KeyCode::KP_3;
            case GLFW_KEY_KP_4: return KeyCode::KP_4;
            case GLFW_KEY_KP_5: return KeyCode::KP_5;
            case GLFW_KEY_KP_6: return KeyCode::KP_6;
            case GLFW_KEY_KP_7: return KeyCode::KP_7;
            case GLFW_KEY_KP_8: return KeyCode::KP_8;
            case GLFW_KEY_KP_9: return KeyCode::KP_9;
            case GLFW_KEY_KP_DECIMAL: return KeyCode::KP_DECIMAL;
            case GLFW_KEY_KP_DIVIDE: return KeyCode::KP_DIVIDE;
            case GLFW_KEY_KP_MULTIPLY: return KeyCode::KP_MULTIPLY;
            case GLFW_KEY_KP_SUBTRACT: return KeyCode::KP_SUBTRACT;
            case GLFW_KEY_KP_ADD: return KeyCode::KP_ADD;
            case GLFW_KEY_KP_ENTER: return KeyCode::KP_ENTER;

            // Mouse Buttons
            case GLFW_MOUSE_BUTTON_1: return KeyCode::MOUSE_1;
            case GLFW_MOUSE_BUTTON_2: return KeyCode::MOUSE_2;
            case GLFW_MOUSE_BUTTON_3: return KeyCode::MOUSE_3;
            case GLFW_MOUSE_BUTTON_4: return KeyCode::MOUSE_4;
            case GLFW_MOUSE_BUTTON_5: return KeyCode::MOUSE_5;
            case GLFW_MOUSE_BUTTON_6: return KeyCode::MOUSE_6;
            case GLFW_MOUSE_BUTTON_7: return KeyCode::MOUSE_7;
            case GLFW_MOUSE_BUTTON_8: return KeyCode::MOUSE_8;

            default:
                return KeyCode::None;
        }
    }
}
