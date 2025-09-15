#include "ui/InteractableWidget.hpp"

#include "client/include/ui/InteractableWidget.hpp"

namespace TechEngineAPI {
    InteractableWidget::InteractableWidget(std::shared_ptr<TechEngine::InteractableWidget>& widget) : Widget(std::dynamic_pointer_cast<TechEngine::InteractableWidget>(widget)) {
    }

    bool InteractableWidget::isClickable() const {
        return dynamic_cast<TechEngine::InteractableWidget*>(m_internalWidget)->isClickable();
    }

    void InteractableWidget::setClickable(bool clickable) {
        dynamic_cast<TechEngine::InteractableWidget*>(m_internalWidget)->setClickable(clickable);
    }
}
