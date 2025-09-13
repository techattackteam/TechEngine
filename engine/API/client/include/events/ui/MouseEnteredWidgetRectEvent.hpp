#pragma once

#include <memory>

#include "common/include/core/ExportDLL.hpp"
#include "common/include/eventSystem/Event.hpp"
#include "client/include/ui/Widget.hpp"


namespace TechEngineAPI {
    class API_DLL MouseEnteredWidgetRectEvent : public Event {
    private:
        std::shared_ptr<Widget> m_widget;

    public:
        explicit MouseEnteredWidgetRectEvent(std::shared_ptr<Widget>& widget) : m_widget(widget) {
        };

        const std::shared_ptr<Widget>& getWidget() {
            return m_widget;
        }
    };
}
