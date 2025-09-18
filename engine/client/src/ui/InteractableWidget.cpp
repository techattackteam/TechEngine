#include "InteractableWidget.hpp"

#include "core/Logger.hpp"
#include "events/ui/MouseClickedInteractableWidgetEvent.hpp"

namespace TechEngine {
    InteractableWidget::InteractableWidget() : Widget() {
        m_type = "Interactable";
        m_name = "Interactable";
        m_category = "Interactable";
        m_description = "Base class for all interactable widgets.";
    }

    InteractableWidget::~InteractableWidget() = default;

    void InteractableWidget::onMouseClick(EventDispatcher& eventDispatcher) {
        if (m_isClickable) {
            eventDispatcher.dispatch<MouseClickedInteractableWidgetEvent>(std::dynamic_pointer_cast<InteractableWidget>(shared_from_this()));
            TE_LOGGER_INFO("InteractableWidget::onMouseClick: Widget '{0}' was clicked.", m_name);
        }
    }

    bool InteractableWidget::isClickable() const {
        return m_isClickable;
    }

    void InteractableWidget::setClickable(bool clickable) {
        this->m_isClickable = clickable;
    }

    void InteractableWidget::serialize(YAML::Emitter& out) const {
        Widget::serialize(out);
        out << YAML::Key << "IsClickable" << YAML::Value << m_isClickable;
    }

    void InteractableWidget::deserialize(const YAML::Node& node, WidgetsRegistry& registry) {
        Widget::deserialize(node, registry);
        if (node["IsClickable"]) {
            m_isClickable = node["IsClickable"].as<bool>();
        }
    }
}
