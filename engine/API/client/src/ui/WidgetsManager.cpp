#include "client/include/ui/WidgetsManager.hpp"
#include "ui/WidgetsRegistry.hpp"

#include "common/include/eventSystem/EventSystem.hpp"
#include "client/include/events/ui/MouseEnteredWidgetRectEvent.hpp"
#include "client/include/events/ui/MouseLeftWidgetRectEvent.hpp"
#include "client/include/ui/Widget.hpp"
#include "core/Logger.hpp"

#include "events/ui/MouseEnteredWidgetRectEvent.hpp"
#include "events/ui/MouseLeftWidgetRectEvent.hpp"


namespace TechEngineAPI {
    void WidgetsManager::init(TechEngine::WidgetsRegistry* widgetsRegistry, TechEngine::EventDispatcher* dispatcher) {
        for (auto& internalWidget: widgetsRegistry->getWidgets()) {
            widgets[internalWidget.get()] = std::make_shared<Widget>(internalWidget);
        }
        for (auto& [internalWidget, widget]: widgets) {
            for (auto& childInternalWidget: internalWidget->m_children) {
                if (widgets.contains(childInternalWidget.get())) {
                    widget->m_children.push_back(widgets[childInternalWidget.get()]);
                    widgets[childInternalWidget.get()]->m_parent = widget;
                }
            }
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

    const std::shared_ptr<Widget>& WidgetsManager::getWidget(const std::string& name) {
        for (auto& [internalWidget, widget]: widgets) {
            if (widget->getName() == name) {
                return widget;
            }
        }
        TE_LOGGER_WARN("Widget with name '{0}' not found in WidgetsManager.", name.c_str());
        return nullptr;
    }
}
