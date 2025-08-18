#include "PanelWidget.hpp"

#include "renderer/ui/UIRenderer.hpp"

namespace TechEngine {
    PanelWidget::PanelWidget() {
        m_name = "PanelWidget";
    }

    void PanelWidget::draw(UIRenderer& renderer) {
        if (m_backgroundColor.a > 0.0f) {
            renderer.drawRectangle({m_finalScreenRect.x, m_finalScreenRect.y}, {m_finalScreenRect.z, m_finalScreenRect.w}, m_backgroundColor);
        }

        Widget::draw(renderer);
    }
}
