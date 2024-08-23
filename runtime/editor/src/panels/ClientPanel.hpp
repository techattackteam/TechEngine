#pragma once

#include "DockPanel.hpp"
#include "GameView.hpp"
#include "InspectorPanel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "SceneView.hpp"
#include "TestPanel.hpp"


namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class ClientPanel : public DockPanel {
    private:
        SystemsRegistry& m_systemRegistry;
        SystemsRegistry& m_clientSystemsRegistry;
        LoggerPanel& loggerPanel;

        TestPanel m_TestPanel;
        SceneView m_sceneView;
        GameView m_gameView;
        SceneHierarchyPanel m_sceneHierarchyPanel;
        InspectorPanel m_inspectorPanel;

    public:
        ClientPanel(SystemsRegistry& systemsRegistry, SystemsRegistry& clientSystemsRegistry, LoggerPanel& loggerPanel);

        void onInit() override;

        void onUpdate() override;

    protected:
        void setupInitialDockingLayout() override;

        void startRunningScene();

        void stopRunningScene();

        void createToolBar() override;
    };
}
