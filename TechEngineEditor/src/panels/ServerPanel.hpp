#pragma once
#include "InspectorPanel.hpp"
#include "Panel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "SceneView.hpp"
#include "core/Server.hpp"
#include "core/Client.hpp"
#include "project/ProjectManager.hpp"

namespace TechEngine {
    class PanelsManager;

    class ServerPanel : public Panel {
    private:
        Server& server;
        ProjectManager& projectManager;
        PanelsManager& panelsManager;


        SceneView sceneView;
        InspectorPanel inspectorPanel;
        SceneHierarchyPanel sceneHierarchyPanel;
        bool m_currentPlaying = false;
        friend class PanelsManager;

    public:
        ServerPanel(PanelsManager& panelsManager, EventDispatcher& eventDispatcher, Server& server, ProjectManager& projectManager, Renderer& renderer);

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
