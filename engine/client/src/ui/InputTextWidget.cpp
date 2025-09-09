#include "InputTextWidget.hpp"

#include "core/Logger.hpp"
#include "input/Key.hpp"
#include "renderer/ui/UIRenderer.hpp"

namespace TechEngine {
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
        if (m_isFocused) {
            if (key.getKeyCode() == BACKSPACE) {
                if (!m_text.empty()) {
                    m_text.pop_back();
                }
            } else {
                m_text += key.getKeyName();
            }
        }
    }
}
