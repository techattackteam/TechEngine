#pragma once
#include "InspectorPanel.hpp"
#include "Panel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "core/Server.hpp"

namespace TechEngine {
    class PanelsManager;

    class ServerPanel : public Panel {
    private:
        Server& server;
        PanelsManager& panelsManager;
        SystemsRegistry& editorRegistry;
        SystemsRegistry& appRegistry;

        SceneHierarchyPanel sceneHierarchyPanel;
        SceneView sceneView;
        InspectorPanel inspectorPanel;
        bool m_currentPlaying = false;
        friend class PanelsManager;

    public:
        ServerPanel(Server& server, SystemsRegistry& editorRegistry, SystemsRegistry& appRegistry, PanelsManager& panels_manager);

        void init() override;

        void onUpdate() override;

        void createToolBar();

        SceneView& getSceneView() {
            return sceneView;
        }

        bool isRunning() const;

    private:
        void startRunningScene();

        void stopRunningScene();
    };
}
