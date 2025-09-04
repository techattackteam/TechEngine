#include "TextWidget.hpp"

#include "renderer/ui/UIRenderer.hpp"

namespace TechEngine {
    TextWidget::TextWidget() : Widget() {
        m_name = "TextWidget";
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
                              m_textColor);
        }

        Widget::draw(renderer);
    }

    const std::string& TextWidget::getText() const {
        return m_text;
    }

    void TextWidget::setText(const std::string& text) {
        this->m_text = text;
    }
}
