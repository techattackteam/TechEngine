#include "PanelWidget.hpp"

#include "renderer/ui/UIRenderer.hpp"

namespace TechEngine {
    PanelWidget::PanelWidget() {
        m_name = "PanelWidget";
    }

    void PanelWidget::draw(UIRenderer& renderer) {
        // Draw the panel's background
        if (m_backgroundColor.a > 0.0f) { // Don't draw if fully transparent
            renderer.drawRectangle({m_finalScreenRect.x, m_finalScreenRect.y}, {m_finalScreenRect.z, m_finalScreenRect.w}, m_backgroundColor);
        }

        // IMPORTANT: Call the base class draw to render children
        Widget::draw(renderer);
    }
}
