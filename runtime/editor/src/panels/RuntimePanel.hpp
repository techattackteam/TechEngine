#pragma once
#include "DockPanel.hpp"
#include "GameView.hpp"
#include "InspectorPanel.hpp"
#include "LoggerPanel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "SceneSettingsPanel.hpp"
#include "SceneView.hpp"
#include "project/ProjectManager.hpp"


namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class RuntimePanel : public DockPanel {
    public:
        enum class InspectorTarget {
            None,
            Entity,
            UIWidget,
            RenderPass
        };

    protected:
        SystemsRegistry& m_editorSystemsRegistry;
        SystemsRegistry& m_appSystemsRegistry;
        LoggerPanel& loggerPanel;

        SceneView m_sceneView;
        SceneHierarchyPanel m_sceneHierarchyPanel;
        SceneSettingsPanel m_sceneSettingsPanel;
        InspectorPanel m_inspectorPanel;
        InspectorTarget m_inspectorTarget = InspectorTarget::None;

        std::vector<Entity> m_selectedEntities;
        HierarchyNode m_selectedNode;
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
