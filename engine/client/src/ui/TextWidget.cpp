#include "TextWidget.hpp"

#include "renderer/ui/UIRenderer.hpp"

namespace TechEngine {
    TextWidget::TextWidget() : Widget() {
        m_name = "Text";
        m_category = "Interactable";
        m_description = "A simple text widget that can display text";

        m_size = {120.0f, 30.0f};
        m_preset = AnchorPreset::TopLeft;
    }

    void TextWidget::draw(UIRenderer& renderer) {
        // Draw the text using the default font
        if (renderer.m_defaultFont) {
            renderer.drawText(renderer.m_defaultFont, m_text,
                              {
                                  m_finalScreenRect.x,
                                  m_finalScreenRect.y
                              },
                              m_fontSize * renderer.getDpiScale(), m_textColor);
        }

        Widget::draw(renderer);
    }

    std::string& TextWidget::getText() {
        return m_text;
    }

    void TextWidget::setText(const std::string& text) {
        this->m_text = text;
    }

    glm::vec4& TextWidget::getColor() {
        return m_textColor;
    }

    float& TextWidget::getFontSize() {
        return m_fontSize;
    }

    bool& TextWidget::isBold() {
        return m_isBold;
    }

    bool& TextWidget::isItalic() {
        return m_isItalic;
    }
}
