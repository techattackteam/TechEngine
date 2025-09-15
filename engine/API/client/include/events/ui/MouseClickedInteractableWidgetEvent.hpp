#pragma once

#include <memory>

#include "common/include/core/ExportDLL.hpp"
#include "common/include/eventSystem/Event.hpp"

#include "client/include/ui/InteractableWidget.hpp"


namespace TechEngineAPI {
    class API_DLL MouseClickedInteractableWidgetEvent : public Event {
    private:
        std::shared_ptr<InteractableWidget> m_widget;

    public:
        explicit MouseClickedInteractableWidgetEvent(std::shared_ptr<InteractableWidget>& widget) : m_widget(widget) {
        };

        const std::shared_ptr<Widget>& getWidget() {
            return m_widget;
        }
    };
}
