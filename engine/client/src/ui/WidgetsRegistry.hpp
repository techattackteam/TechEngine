#pragma once

#include "Widget.hpp"
#include "systems/System.hpp"

#include <string>
#include <vector>


namespace TechEngine {
    class KeyPressedEvent;
    class MouseMoveEvent;

    class CLIENT_DLL WidgetsRegistry : public System {
    private:
        std::vector<Widget> m_widgetsTemplates;
        std::vector<std::shared_ptr<Widget>> m_widgets;
        SystemsRegistry& m_systemsRegistry;
        std::shared_ptr<Widget> m_focusedWidget = nullptr;
        std::shared_ptr<Widget> m_currentlyHoveredWidget = nullptr;

    public:
        WidgetsRegistry(SystemsRegistry& systemsRegistry);

        void init() override;

        void onUpdate() override;

        bool loadJson(const std::string& jsonFilePath);

        std::shared_ptr<Widget> createWidget(const std::string& name);

        const std::vector<Widget>& getWidgetsTemplates() const;

        std::vector<std::shared_ptr<Widget>>& getWidgets();

        void calculateWidgetLayout(const std::shared_ptr<Widget>& widget);

    private:
        void onMouseMoveEvent(const ::std::shared_ptr<MouseMoveEvent>& event);

        void onMousePressedEvent(const std::shared_ptr<KeyPressedEvent>& event);

        void onKeyPressedEvent(const std::shared_ptr<KeyPressedEvent>& event);
    };
}
