#include <ui/PanelWidget.hpp>

#include "client/include/ui/PanelWidget.hpp"

namespace TechEngineAPI {
    PanelWidget::PanelWidget(std::shared_ptr<TechEngine::PanelWidget>& widget) : Widget(std::dynamic_pointer_cast<TechEngine::PanelWidget>(widget)) {
    }

    const glm::vec4& PanelWidget::getBackgroundColor() {
        return dynamic_cast<TechEngine::PanelWidget*>(m_internalWidget)->getBackgroundColor();
    }

    void PanelWidget::setBackgroundColor(const glm::vec4& color) {
        return dynamic_cast<TechEngine::PanelWidget*>(m_internalWidget)->setBackgroundColor(color);
    }
}
