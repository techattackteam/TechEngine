#include "client/include/ui/WidgetsManager.hpp"
#include "ui/WidgetsRegistry.hpp"

#include "common/include/eventSystem/EventSystem.hpp"
#include "client/include/events/ui/MouseEnteredWidgetRectEvent.hpp"
#include "client/include/events/ui/MouseLeftWidgetRectEvent.hpp"
#include "client/include/ui/Widget.hpp"

#include "events/ui/MouseEnteredWidgetRectEvent.hpp"
#include "events/ui/MouseLeftWidgetRectEvent.hpp"


namespace TechEngineAPI {
    void WidgetsManager::init(TechEngine::WidgetsRegistry* widgetsRegistry, TechEngine::EventDispatcher* dispatcher) {
        for (auto& internalWidget: widgetsRegistry->getWidgets()) {
            widgets[internalWidget.get()] = std::make_shared<Widget>(internalWidget);
        }
        dispatcher->subscribe<TechEngine::MouseEnteredWidgetRectEvent>([](const std::shared_ptr<TechEngine::Event>& event) {
            const std::shared_ptr<TechEngine::Widget>& internalWidget = dynamic_cast<TechEngine::MouseEnteredWidgetRectEvent*>(event.get())->getWidget();

            EventSystem::dispatch<MouseEnteredWidgetRectEvent>(widgets[internalWidget.get()]);
        });

        dispatcher->subscribe<TechEngine::MouseLeftWidgetRectEvent>([](const std::shared_ptr<TechEngine::Event>& event) {
            const std::shared_ptr<TechEngine::Widget>& internalWidget = dynamic_cast<TechEngine::MouseLeftWidgetRectEvent*>(event.get())->getWidget();
            EventSystem::dispatch<MouseLeftWidgetRectEvent>(widgets[internalWidget.get()]);
        });
    }

    void WidgetsManager::shutdown() {
        widgets.clear();
    }
}
