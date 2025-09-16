#pragma once

#include <memory>

#include "core/ExportDLL.hpp"
#include "systems/System.hpp"
#include "Widget.hpp"
#include "commands/ICommand.hpp"

namespace TechEngine {
    class KeyPressedEvent;
    class MouseMoveEvent;

    class CLIENT_DLL WidgetsRegistry : public System {
    public:
        struct CustomWidgetTemplate {
            std::string type;
            std::string name;
            std::string category;
            std::string description;
            std::vector<CustomWidgetTemplate> children;
            std::unordered_map<std::string, std::tuple<std::string, std::string>> properties; // Key: property name, Value: (type, default value)
        };

        // A command queue for deferred actions
        enum class ActionType { Move, Reparent };

        struct PendingAction {
            ActionType type;
            std::shared_ptr<Widget> subject; // The widget being moved
            std::shared_ptr<Widget> destination; // The new parent (nullptr for root)
            int widgetIndex; // The new index in the main widgets list
            int siblingIndex; // The new index among siblings
        };

    private:
        std::vector<CustomWidgetTemplate> m_widgetsTemplates;
        std::vector<std::shared_ptr<Widget>> m_widgets;
        std::vector<std::shared_ptr<Widget>> m_rootWidgets;
        SystemsRegistry& m_systemsRegistry;

        std::shared_ptr<Widget> m_focusedWidget = nullptr;
        std::shared_ptr<Widget> m_currentlyHoveredWidget = nullptr;

        std::vector<std::unique_ptr<ICommand>> m_commandQueue;

    public:
        explicit WidgetsRegistry(SystemsRegistry& systemsRegistry);

        WidgetsRegistry(const WidgetsRegistry&) = delete;

        WidgetsRegistry& operator=(const WidgetsRegistry&) = delete;

        void init() override;

        void onUpdate() override;

        bool loadYaml(const std::string& jsonFilePath);

        std::shared_ptr<Widget> createWidget(const std::shared_ptr<Widget>& parent, const std::string& name, bool custom);

        void calculateWidgetLayout(const std::shared_ptr<Widget>& widget);

        void queueCommand(std::unique_ptr<ICommand> command);

        void applyCommands();

        void performWidgetMove(const std::shared_ptr<Widget>& widgetToMove, const std::shared_ptr<Widget>& newParent, int newSiblingIndex);

        void rebuildFlattenedList();

        std::vector<std::shared_ptr<Widget>>& getWidgets();

        const std::vector<CustomWidgetTemplate>& getWidgetTemplates();

        const std::vector<std::shared_ptr<Widget>>& getRootWidgets() const;

    private:
        void createDefaultWidgetYamlFile();

        void onMouseMoveEvent(const ::std::shared_ptr<MouseMoveEvent>& event);

        void onMousePressedEvent(const std::shared_ptr<KeyPressedEvent>& event);

        void onKeyPressedEvent(const std::shared_ptr<KeyPressedEvent>& event);

        std::shared_ptr<Widget> createBaseWidget(const std::string& typeName, const std::string& name);

        std::shared_ptr<Widget> createCustomWidget(const CustomWidgetTemplate& widgetTemplate, const std::string& name);
    };
}
