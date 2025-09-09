#include "InputTextWidget.hpp"

#include "core/Logger.hpp"
#include "input/Input.hpp"
#include "input/Key.hpp"
#include "renderer/ui/UIRenderer.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    InputTextWidget::InputTextWidget(SystemsRegistry& systemsRegistry) : Widget(), m_systemsRegistry(systemsRegistry) {
    }

    InputTextWidget::~InputTextWidget() = default;

    void InputTextWidget::update(float deltaTime) {
        if (m_isFocused) {
            m_cursorBlinkTimer += deltaTime;
            if (m_cursorBlinkTimer >= 0.5f) {
                m_isCursorVisible = !m_isCursorVisible;
                m_cursorBlinkTimer = 0.0f;
            }
        }
    }

    void InputTextWidget::draw(UIRenderer& renderer) {
        // m_finalScreenRect is calculated by the layout system

        // 1. Draw the background
        renderer.drawRectangle({m_finalScreenRect.x, m_finalScreenRect.y}, {m_finalScreenRect.z, m_finalScreenRect.w}, m_backgroundColor);

        // 2. Draw the border (color changes if focused)
        //glm::vec4 borderColor = m_isFocused ? m_focusedBorderColor : m_borderColor;
        //renderer.drawBorder(m_finalScreenRect, borderColor, 1.0f); // Assuming a drawBorder function

        // 3. Determine which text and color to use
        std::string textToDraw;
        glm::vec4 colorToUse;

        // Show placeholder if the field is empty AND not focused
        if (m_text.empty() && !m_isFocused) {
            textToDraw = m_placeholderText;
            colorToUse = m_placeholderColor;
        } else {
            textToDraw = m_text;
            colorToUse = m_textColor;
        }

        // 4. Draw the Text
        // We need some padding. Let's assume 5 pixels.
        // NOTE: Your renderer will need to know the current dpiScale to draw the font at the correct size!
        float scaledFontSize = m_fontSize * renderer.getDpiScale();
        glm::vec2 textPosition = {m_finalScreenRect.x + 5, m_finalScreenRect.y + 5};
        renderer.drawText(renderer.m_defaultFont, textToDraw,
                          {
                              textPosition.x,
                              textPosition.y
                          },
                          scaledFontSize, colorToUse);

        // 5. Draw the Blinking Cursor
        if (m_isFocused && m_isCursorVisible) {
            //float textWidth = renderer.measureTextWidth(textToDraw, scaledFontSize);
            //float cursorX = m_finalScreenRect.x + 5 + textWidth;
            //float cursorY_start = m_finalScreenRect.y + 4;
            //float cursorY_end = m_finalScreenRect.y + m_finalScreenRect.w - 4;
            //
            //renderer.drawLine({cursorX, cursorY_start}, {cursorX, cursorY_end}, m_textColor, 1.0f);
        }
    }

    void InputTextWidget::gainFocus() {
        m_isFocused = true;
        m_isCursorVisible = true;
        m_cursorBlinkTimer = 0.0f;
    }

    void InputTextWidget::loseFocus() {
        m_isFocused = false;
        m_isCursorVisible = false;
        m_cursorBlinkTimer = 0.0f;
    }

    void InputTextWidget::onKeyPressed(Key& key) {
        if (!m_isFocused) {
            return;
        }

        // Get modifier key states once at the beginning
        Input& input = m_systemsRegistry.getSystem<Input>();
        bool isShift = input.isKeyPressed(LEFT_SHIFT) || input.isKeyPressed(RIGHT_SHIFT);
        bool isCaps = input.isCapsLockActive();

        switch (key.getKeyCode()) {
            // === 1. Action Keys (handled separately) ===
            case BACKSPACE:
                if (!m_text.empty()) {
                    m_text.pop_back();
                }
                break;

            // === 2. Whitespace Characters ===
            case ENTER:
            case KP_ENTER:
                m_text += "\n";
                break;
            case TAB:
                m_text += "\t"; // Usually 4 or 8 spaces, handled by renderer
                break;
            case SPACE:
                m_text += " ";
                break;

            // === 3. Letter Characters ===
            // The XOR (^) correctly handles the logic for caps lock and shift.
            // (Shift AND NOT Caps) OR (NOT Shift AND Caps)
            case A: m_text += (isShift ^ isCaps) ? 'A' : 'a';
                break;
            case B: m_text += (isShift ^ isCaps) ? 'B' : 'b';
                break;
            case C: m_text += (isShift ^ isCaps) ? 'C' : 'c';
                break;
            case D: m_text += (isShift ^ isCaps) ? 'D' : 'd';
                break;
            case E: m_text += (isShift ^ isCaps) ? 'E' : 'e';
                break;
            case F: m_text += (isShift ^ isCaps) ? 'F' : 'f';
                break;
            case G: m_text += (isShift ^ isCaps) ? 'G' : 'g';
                break;
            case H: m_text += (isShift ^ isCaps) ? 'H' : 'h';
                break;
            case I: m_text += (isShift ^ isCaps) ? 'I' : 'i';
                break;
            case J: m_text += (isShift ^ isCaps) ? 'J' : 'j';
                break;
            case K: m_text += (isShift ^ isCaps) ? 'K' : 'k';
                break;
            case L: m_text += (isShift ^ isCaps) ? 'L' : 'l';
                break;
            case M: m_text += (isShift ^ isCaps) ? 'M' : 'm';
                break;
            case N: m_text += (isShift ^ isCaps) ? 'N' : 'n';
                break;
            case O: m_text += (isShift ^ isCaps) ? 'O' : 'o';
                break;
            case P: m_text += (isShift ^ isCaps) ? 'P' : 'p';
                break;
            case Q: m_text += (isShift ^ isCaps) ? 'Q' : 'q';
                break;
            case R: m_text += (isShift ^ isCaps) ? 'R' : 'r';
                break;
            case S: m_text += (isShift ^ isCaps) ? 'S' : 's';
                break;
            case T: m_text += (isShift ^ isCaps) ? 'T' : 't';
                break;
            case U: m_text += (isShift ^ isCaps) ? 'U' : 'u';
                break;
            case V: m_text += (isShift ^ isCaps) ? 'V' : 'v';
                break;
            case W: m_text += (isShift ^ isCaps) ? 'W' : 'w';
                break;
            case X: m_text += (isShift ^ isCaps) ? 'X' : 'x';
                break;
            case Y: m_text += (isShift ^ isCaps) ? 'Y' : 'y';
                break;
            case Z: m_text += (isShift ^ isCaps) ? 'Z' : 'z';
                break;

            // === 4. Number Characters (Top Row) ===
            case ZERO: m_text += isShift ? ')' : '0';
                break;
            case ONE: m_text += isShift ? '!' : '1';
                break;
            case TWO: m_text += isShift ? '@' : '2';
                break;
            case THREE: m_text += isShift ? '#' : '3';
                break;
            case FOUR: m_text += isShift ? '$' : '4';
                break;
            case FIVE: m_text += isShift ? '%' : '5';
                break;
            case SIX: m_text += isShift ? '^' : '6';
                break;
            case SEVEN: m_text += isShift ? '&' : '7';
                break;
            case EIGHT: m_text += isShift ? '*' : '8';
                break;
            case NINE: m_text += isShift ? '(' : '9';
                break;

            // === 5. Punctuation and Symbol Keys ===
            case GRAVE_ACCENT: m_text += isShift ? '~' : '`';
                break;
            case MINUS: m_text += isShift ? '_' : '-';
                break;
            case EQUAL: m_text += isShift ? '+' : '=';
                break;
            case LEFT_BRACKET: m_text += isShift ? '{' : '[';
                break;
            case RIGHT_BRACKET: m_text += isShift ? '}' : ']';
                break;
            case BACKSLASH: m_text += isShift ? '|' : '\\';
                break;
            case SEMICOLON: m_text += isShift ? ':' : ';';
                break;
            case APOSTROPHE: m_text += isShift ? '"' : '\'';
                break;
            case COMMA: m_text += isShift ? '<' : ',';
                break;
            case PERIOD: m_text += isShift ? '>' : '.';
                break;
            case SLASH: m_text += isShift ? '?' : '/';
                break;

            // === 6. Numpad Keys (Shift doesn't apply) ===
            case KP_0: m_text += '0';
                break;
            case KP_1: m_text += '1';
                break;
            case KP_2: m_text += '2';
                break;
            case KP_3: m_text += '3';
                break;
            case KP_4: m_text += '4';
                break;
            case KP_5: m_text += '5';
                break;
            case KP_6: m_text += '6';
                break;
            case KP_7: m_text += '7';
                break;
            case KP_8: m_text += '8';
                break;
            case KP_9: m_text += '9';
                break;
            case KP_DECIMAL: m_text += '.';
                break;
            case KP_DIVIDE: m_text += '/';
                break;
            case KP_MULTIPLY: m_text += '*';
                break;
            case KP_SUBTRACT: m_text += '-';
                break;
            case KP_ADD: m_text += '+';
                break;

            // === 7. Ignored Keys ===
            // Any key not listed above will be caught by the default case and ignored.
            // This includes F-keys, arrows, modifiers, Home, End, etc.
            default:
                // Do nothing for non-textual keys.
                break;
        }
    }
}
