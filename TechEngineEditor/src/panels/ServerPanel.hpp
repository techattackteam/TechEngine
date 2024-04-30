#pragma once
#include "InspectorPanel.hpp"
#include "Panel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "SceneView.hpp"
#include "core/Server.hpp"
#include "core/Client.hpp"

namespace TechEngine {
    class PanelsManager;

    class ServerPanel : public Panel {
    private:
        PanelsManager& panelsManager;
        Server& server;
        SceneView sceneView;
        InspectorPanel inspectorPanel;
        SceneHierarchyPanel sceneHierarchyPanel;
        bool m_currentPlaying = false;

    public:
        ServerPanel(PanelsManager& panelsManager, Server& server, Renderer& renderer);

        void onUpdate() override;

        void createToolBar();

        SceneView& getSceneView() {
            return sceneView;
        }
    };
}
