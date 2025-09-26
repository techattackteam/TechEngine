#pragma once

#include "events/Event.hpp"
#include "ui/InteractableWidget.hpp"

namespace TechEngine {
    class MouseClickedInteractableWidgetEvent : public Event {
    private:
        std::shared_ptr<InteractableWidget> m_widget;

    public:
        explicit MouseClickedInteractableWidgetEvent(const std::shared_ptr<InteractableWidget>& widget) : m_widget(widget) {
        }

        const std::shared_ptr<InteractableWidget>& getWidget() {
            return m_widget;
        }
    };
}
