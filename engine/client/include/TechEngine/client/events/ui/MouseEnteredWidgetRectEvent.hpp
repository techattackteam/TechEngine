#pragma once


#include "events/Event.hpp"
#include "ui/Widget.hpp"

namespace TechEngine {
    class MouseEnteredWidgetRectEvent : public Event {
    private:
        std::shared_ptr<Widget> m_widget;

    public:
        explicit MouseEnteredWidgetRectEvent(const std::shared_ptr<Widget>& widget) : m_widget(widget) {
        }

        const std::shared_ptr<Widget>& getWidget() {
            return m_widget;
        }
    };
}
