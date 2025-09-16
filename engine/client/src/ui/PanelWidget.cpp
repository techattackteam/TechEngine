#include "PanelWidget.hpp"

#include "renderer/ui/UIRenderer.hpp"

namespace TechEngine {
    PanelWidget::PanelWidget() {
        m_name = "Panel";
        m_category = "Container";
        m_description = "A simple panel widget that can contain other widgets and has a background color";

        m_properties["BackgroundColor"] = "vec4";
    }

    void PanelWidget::draw(UIRenderer& renderer) {
        if (m_backgroundColor.a > 0.0f) {
            renderer.drawRectangle({m_finalScreenRect.x, m_finalScreenRect.y}, {m_finalScreenRect.z, m_finalScreenRect.w}, m_backgroundColor);
        }

        Widget::draw(renderer);
    }

    const glm::vec4& PanelWidget::getBackgroundColor() {
        return m_backgroundColor;
    }

    void PanelWidget::setBackgroundColor(const glm::vec4& color) {
        this->m_backgroundColor = color;
    }
}
