#include "client/include/ui/TextWidget.hpp"

#include "ui/TextWidget.hpp"

namespace TechEngineAPI {
    TextWidget::TextWidget(std::shared_ptr<TechEngine::TextWidget>& widget) : Widget(std::dynamic_pointer_cast<TechEngine::TextWidget>(widget)) {
    }

    std::string& TextWidget::getText() {
        return dynamic_cast<TechEngine::TextWidget*>(m_internalWidget)->getText();
    }

    void TextWidget::setText(const std::string& text) {
        return dynamic_cast<TechEngine::TextWidget*>(m_internalWidget)->setText(text);
    }

    glm::vec4& TextWidget::getColor() {
        return dynamic_cast<TechEngine::TextWidget*>(m_internalWidget)->getColor();
    }
}
