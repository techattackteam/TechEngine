#include "client/include/input/Key.hpp"

#include "input/Key.hpp"

namespace TechEngineAPI {
    Key::Key(KeyCode keyCode) : keyCode(keyCode) {
    }

    KeyCode& Key::getKeyCode() {
        return keyCode;
    }

    std::string Key::getKeyName() {
        switch (keyCode) {
            case Q:
                return "Q";
            case W:
                return "W";
            case E:
                return "E";
            case R:
                return "R";
            case T:
                return "T";
            case Y:
                return "Y";
            case U:
                return "U";
            case I:
                return "I";
            case O:
                return "O";
            case P:
                return "P";
            case A:
                return "A";
            case S:
                return "S";
            case D:
                return "D";
            case F:
                return "F";
            case G:
                return "G";
            case H:
                return "H";
            case J:
                return "J";
            case K:
                return "K";
            case L:
                return "L";
            case Z:
                return "Z";
            case X:
                return "X";
            case C:
                return "C";
            case V:
                return "V";
            case B:
                return "B";
            case N:
                return "N";
            case M:
                return "M";
            case ZERO:
                return "0";
            case ONE:
                return "1";
            case TWO:
                return "2";
            case THREE:
                return "3";
            case FOUR:
                return "4";
            case FIVE:
                return "5";
            case SIX:
                return "6";
            case SEVEN:
                return "7";
            case EIGHT:
                return "8";
            case NINE:
                return "9";
            case ESC:
                return "escape";
            case ARROW_UP:
                return "UP";
            case ARROW_DOWN:
                return "DOWN";
            case ARROW_RIGHT:
                return "RIGHT";
            case ARROW_LEFT:
                return "LEFT";
            case MOUSE_1:
                return "MOUSE_1";
            case MOUSE_2:
                return "MOUSE_2";
            case MOUSE_3:
                return "MOUSE_3";
            case SPACE:
                return "SPACE";
            case ENTER:
                return "ENTER";
            case TAB:
                return "TAB";
            case BACKSPACE:
                return "BACKSPACE";
            case DEL:
                return "DELETE";
            case LEFT_SHIFT:
                return "LEFT_SHIFT";
            case RIGHT_SHIFT:
                return "RIGHT_SHIFT";
            case LEFT_CTRL:
                return "LEFT_CTRL";
            case RIGHT_CTRL:
                return "RIGHT_CTRL";
            case LEFT_ALT:
                return "LEFT_ALT";
            case RIGHT_ALT:
                return "RIGHT_ALT";
            default:
                return "";
        }
    }
}
