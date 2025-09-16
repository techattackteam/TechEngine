#pragma once

#include "DockPanel.hpp"
#include "UIHierarchyPanel.hpp"
#include "UIView.hpp"
#include "UIInspector.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/WidgetsRegistry.hpp"


namespace TechEngine {
    class GameView;

    class UIEditor : public DockPanel {
    private:
        UIView m_uiView;
        UIHierarchyPanel m_uiHierarchy;
        UIInspector m_uiInspector;
        GameView& m_gameView;

        SystemsRegistry& m_appSystemsRegistry;

    public:
        std::shared_ptr<Widget> m_selectedWidget = nullptr;

        UIEditor(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry, GameView& sceneView);

        void onInit() override;

        void onUpdate() override;

        //std::shared_ptr<Widget> createWidget(const std::shared_ptr<Widget>& parent, const std::string& type, const std::string& name);

        bool deleteWidget(const std::shared_ptr<Widget>& widget);

        void setSelectedWidget(const std::shared_ptr<Widget>& widget);

        std::shared_ptr<Widget> getSelectedWidget() const {
            return m_selectedWidget;
        }

        WidgetsRegistry& getWidgetsRegistry() {
            return m_appSystemsRegistry.getSystem<WidgetsRegistry>();
        }

        GameView& getGameView() const {
            return m_gameView;
        }

    protected:
        void setupInitialDockingLayout() override;

    private:
    };
}
