#pragma once
#include "DockPanel.hpp"
#include "GameView.hpp"
#include "InspectorPanel.hpp"
#include "LoggerPanel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "SceneView.hpp"
#include "project/ProjectManager.hpp"


namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class RuntimePanel : public DockPanel {
    protected:
        SystemsRegistry& m_editorSystemsRegistry;
        SystemsRegistry& m_appSystemsRegistry;
        LoggerPanel& loggerPanel;

        SceneView m_sceneView;
        //GameView m_gameView;
        //SceneHierarchyPanel m_sceneHierarchyPanel;
        //InspectorPanel m_inspectorPanel;
        std::vector<Entity> m_selectedEntities;

        ProjectType m_projectType;

    public:
        RuntimePanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry, LoggerPanel& loggerPanel);

        ~RuntimePanel() override = default;

        void onInit() override;

        void onUpdate() override;

    protected:
        void setupInitialDockingLayout() override;

        void startRunningScene();

        void stopRunningScene();

        void createToolBar() override;
    };
}
