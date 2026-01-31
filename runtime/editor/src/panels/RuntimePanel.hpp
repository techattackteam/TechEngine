#pragma once
#include "DockPanel.hpp"
#include "EntityHierarchyPanel.hpp"
#include "WidgetHierarchyPanel.hpp"
#include "RenderPassHierarchyPanel.hpp"
#include "GameView.hpp"
#include "InspectorPanel.hpp"
#include "LoggerPanel.hpp"
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

        // New hierarchy panels
        EntityHierarchyPanel m_entityHierarchyPanel;
        //WidgetHierarchyPanel m_widgetHierarchyPanel;
        //RenderPassHierarchyPanel m_renderPassHierarchyPanel;

        InspectorPanel m_inspectorPanel;
        InspectorTarget m_inspectorTarget = InspectorTarget::None;

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
