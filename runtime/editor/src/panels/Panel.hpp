#pragma once

#include "input/Key.hpp"
#include <imgui.h>
#include <string>
#include <variant>
#include <vector>
#include <glm/vec2.hpp>

#include "core/UUID.hpp"


namespace TechEngine {
    class SystemsRegistry;
}

namespace TechEngine {
    class Panel {
    protected:
        inline static int nextID = 0;
        int id;
        std::string m_name;

        bool m_isVisible = true;
        bool m_isPanelHovered = false;
        bool m_isPanelFocused = false;
        bool m_isDragging = false;
        bool m_isResizing = false;
        bool m_isMoving = false;

        ImVec2 m_lastSize;
        ImVec2 m_lastPosition;

        ImGuiWindowClass* m_parentDockSpaceClass = nullptr; // from parent otherwise editor window
        ImGuiWindowFlags m_windowFlags = ImGuiWindowFlags_None;
        std::vector<std::tuple<ImGuiStyleVar, std::variant<ImVec2, float>>> m_styleVars;

        SystemsRegistry& m_editorSystemsRegistry;

    public:
        Panel(SystemsRegistry& editorSystemsRegistry);

        virtual ~Panel() = default;

        virtual void init(const std::string& name, ImGuiWindowClass* parentDockSpaceClass, bool isVisible = true);

        virtual void update();

        virtual void onInit() = 0;

        virtual void onUpdate() = 0;

        void processInput();

        virtual void processMouseDragging(glm::vec2 delta, unsigned long long mouseButtons);

        virtual void processMouseMoving(glm::vec2 delta);

        virtual void processMouseScroll(float yOffset);

        virtual void processKeyPressed(Key key);

        virtual void processKeyReleased(Key key);

        virtual void processKeyHold(Key key);

        static KeyCode ImGuiKeyToEngineKeyCode(ImGuiKey imguiKey) {
            switch (imguiKey) {
                // Letters
                case ImGuiKey_A: return KeyCode::A;
                case ImGuiKey_B: return KeyCode::B;
                case ImGuiKey_C: return KeyCode::C;
                case ImGuiKey_D: return KeyCode::D;
                case ImGuiKey_E: return KeyCode::E;
                case ImGuiKey_F: return KeyCode::F;
                case ImGuiKey_G: return KeyCode::G;
                case ImGuiKey_H: return KeyCode::H;
                case ImGuiKey_I: return KeyCode::I;
                case ImGuiKey_J: return KeyCode::J;
                case ImGuiKey_K: return KeyCode::K;
                case ImGuiKey_L: return KeyCode::L;
                case ImGuiKey_M: return KeyCode::M;
                case ImGuiKey_N: return KeyCode::N;
                case ImGuiKey_O: return KeyCode::O;
                case ImGuiKey_P: return KeyCode::P;
                case ImGuiKey_Q: return KeyCode::Q;
                case ImGuiKey_R: return KeyCode::R;
                case ImGuiKey_S: return KeyCode::S;
                case ImGuiKey_T: return KeyCode::T;
                case ImGuiKey_U: return KeyCode::U;
                case ImGuiKey_V: return KeyCode::V;
                case ImGuiKey_W: return KeyCode::W;
                case ImGuiKey_X: return KeyCode::X;
                case ImGuiKey_Y: return KeyCode::Y;
                case ImGuiKey_Z: return KeyCode::Z;

                // Numbers
                case ImGuiKey_0: return KeyCode::ZERO;
                case ImGuiKey_1: return KeyCode::ONE;
                case ImGuiKey_2: return KeyCode::TWO;
                case ImGuiKey_3: return KeyCode::THREE;
                case ImGuiKey_4: return KeyCode::FOUR;
                case ImGuiKey_5: return KeyCode::FIVE;
                case ImGuiKey_6: return KeyCode::SIX;
                case ImGuiKey_7: return KeyCode::SEVEN;
                case ImGuiKey_8: return KeyCode::EIGHT;
                case ImGuiKey_9: return KeyCode::NINE;

                // Function Keys
                case ImGuiKey_F1: return KeyCode::F1;
                case ImGuiKey_F2: return KeyCode::F2;
                case ImGuiKey_F3: return KeyCode::F3;
                case ImGuiKey_F4: return KeyCode::F4;
                case ImGuiKey_F5: return KeyCode::F5;
                case ImGuiKey_F6: return KeyCode::F6;
                case ImGuiKey_F7: return KeyCode::F7;
                case ImGuiKey_F8: return KeyCode::F8;
                case ImGuiKey_F9: return KeyCode::F9;
                case ImGuiKey_F10: return KeyCode::F10;
                case ImGuiKey_F11: return KeyCode::F11;
                case ImGuiKey_F12: return KeyCode::F12;

                // Arrow Keys
                case ImGuiKey_UpArrow: return KeyCode::ARROW_UP;
                case ImGuiKey_DownArrow: return KeyCode::ARROW_DOWN;
                case ImGuiKey_LeftArrow: return KeyCode::ARROW_LEFT;
                case ImGuiKey_RightArrow: return KeyCode::ARROW_RIGHT;

                // Special & Formatting
                case ImGuiKey_Escape: return KeyCode::ESC;
                case ImGuiKey_Space: return KeyCode::SPACE;
                case ImGuiKey_Enter: return KeyCode::ENTER;
                case ImGuiKey_Tab: return KeyCode::TAB;
                case ImGuiKey_Backspace: return KeyCode::BACKSPACE;
                case ImGuiKey_Delete: return KeyCode::DEL;
                case ImGuiKey_Insert: return KeyCode::INSERT;
                case ImGuiKey_Home: return KeyCode::HOME;
                case ImGuiKey_End: return KeyCode::END;
                case ImGuiKey_PageUp: return KeyCode::PAGE_UP;
                case ImGuiKey_PageDown: return KeyCode::PAGE_DOWN;

                // Modifier Keys
                case ImGuiKey_LeftShift: return KeyCode::LEFT_SHIFT;
                case ImGuiKey_RightShift: return KeyCode::RIGHT_SHIFT;
                case ImGuiKey_LeftCtrl: return KeyCode::LEFT_CTRL;
                case ImGuiKey_RightCtrl: return KeyCode::RIGHT_CTRL;
                case ImGuiKey_LeftAlt: return KeyCode::LEFT_ALT;
                case ImGuiKey_RightAlt: return KeyCode::RIGHT_ALT;
                case ImGuiKey_CapsLock: return KeyCode::CAPS_LOCK;

                // Punctuation & Symbols
                case ImGuiKey_GraveAccent: return KeyCode::GRAVE_ACCENT;
                case ImGuiKey_Minus: return KeyCode::MINUS;
                case ImGuiKey_Equal: return KeyCode::EQUAL;
                case ImGuiKey_LeftBracket: return KeyCode::LEFT_BRACKET;
                case ImGuiKey_RightBracket: return KeyCode::RIGHT_BRACKET;
                case ImGuiKey_Backslash: return KeyCode::BACKSLASH;
                case ImGuiKey_Semicolon: return KeyCode::SEMICOLON;
                case ImGuiKey_Apostrophe: return KeyCode::APOSTROPHE;
                case ImGuiKey_Comma: return KeyCode::COMMA;
                case ImGuiKey_Period: return KeyCode::PERIOD;
                case ImGuiKey_Slash: return KeyCode::SLASH;

                // Numpad Keys
                case ImGuiKey_Keypad0: return KeyCode::KP_0;
                case ImGuiKey_Keypad1: return KeyCode::KP_1;
                case ImGuiKey_Keypad2: return KeyCode::KP_2;
                case ImGuiKey_Keypad3: return KeyCode::KP_3;
                case ImGuiKey_Keypad4: return KeyCode::KP_4;
                case ImGuiKey_Keypad5: return KeyCode::KP_5;
                case ImGuiKey_Keypad6: return KeyCode::KP_6;
                case ImGuiKey_Keypad7: return KeyCode::KP_7;
                case ImGuiKey_Keypad8: return KeyCode::KP_8;
                case ImGuiKey_Keypad9: return KeyCode::KP_9;
                case ImGuiKey_KeypadDecimal: return KeyCode::KP_DECIMAL;
                case ImGuiKey_KeypadDivide: return KeyCode::KP_DIVIDE;
                case ImGuiKey_KeypadMultiply: return KeyCode::KP_MULTIPLY;
                case ImGuiKey_KeypadSubtract: return KeyCode::KP_SUBTRACT;
                case ImGuiKey_KeypadAdd: return KeyCode::KP_ADD;
                case ImGuiKey_KeypadEnter: return KeyCode::KP_ENTER;

                // Mouse Buttons
                case ImGuiKey_MouseLeft: return KeyCode::MOUSE_1;
                case ImGuiKey_MouseRight: return KeyCode::MOUSE_2;
                case ImGuiKey_MouseMiddle: return KeyCode::MOUSE_3;
                case ImGuiKey_MouseX1: return KeyCode::MOUSE_4;
                case ImGuiKey_MouseX2: return KeyCode::MOUSE_5;

                // Unmapped keys will fall through to the default
                default:
                    return KeyCode::None;
            }
        }


        const std::string& getName() const {
            return m_name;
        }

        const int& getId() const {
            return id;
        }
    };
}
