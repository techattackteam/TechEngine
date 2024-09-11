#pragma once

#include "DockPanel.hpp"
#include "GameView.hpp"
#include "InspectorPanel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "SceneView.hpp"

#include "components/Components.hpp"

namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class ClientPanel : public DockPanel {
    private:
        SystemsRegistry& m_systemRegistry;
        SystemsRegistry& m_clientSystemsRegistry;
        LoggerPanel& loggerPanel;

        SceneView m_sceneView;
        GameView m_gameView;
        SceneHierarchyPanel m_sceneHierarchyPanel;
        InspectorPanel m_inspectorPanel;
        std::vector<Entity> m_selectedEntities;

    public:
        ClientPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& clientSystemsRegistry, LoggerPanel& loggerPanel);

        ~ClientPanel() override = default;

        void onInit() override;

        void onUpdate() override;

    protected:
        void setupInitialDockingLayout() override;

        void startRunningScene();

        void stopRunningScene();

        void createToolBar() override;
    };
}
