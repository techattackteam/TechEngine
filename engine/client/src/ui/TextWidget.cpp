#include "TextWidget.hpp"

#include "renderer/ui/UIRenderer.hpp"

namespace TechEngine {
    TextWidget::TextWidget() : Widget() {
        m_type = "Text";
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

    void TextWidget::serialize(YAML::Emitter& out) const {
        Widget::serialize(out);
        out << YAML::Key << "Text" << YAML::Value << m_text;
        out << YAML::Key << "TextColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << m_textColor.x << m_textColor.y << m_textColor.z << m_textColor.w << YAML::EndSeq;
        out << YAML::Key << "FontSize" << YAML::Value << m_fontSize;
        out << YAML::Key << "IsBold" << YAML::Value << m_isBold;
        out << YAML::Key << "IsItalic" << YAML::Value << m_isItalic;
    }

    void TextWidget::deserialize(const YAML::Node& node, WidgetsRegistry& registry) {
        Widget::deserialize(node, registry);
        if (node["Text"]) {
            this->m_text = node["Text"].as<std::string>();
        }
        if (node["TextColor"]) {
            this->m_textColor = node["TextColor"].as<glm::vec4>();
        }
        if (node["FontSize"]) {
            this->m_fontSize = node["FontSize"].as<float>();
        }
        if (node["IsBold"]) {
            this->m_isBold = node["IsBold"].as<bool>();
        }
        if (node["IsItalic"]) {
            this->m_isItalic = node["IsItalic"].as<bool>();
        }
    }
}
