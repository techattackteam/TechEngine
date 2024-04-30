#pragma once

#include "Panel.hpp"
#include "GameView.hpp"
#include "InspectorPanel.hpp"
#include "SceneView.hpp"
#include "SceneHierarchyPanel.hpp"
#include "core/Client.hpp"

namespace TechEngine {
    class PanelsManager;
}

namespace TechEngine {
    enum CompileMode;

    class ClientPanel : public Panel {
    private:
        Client& client;
        ProjectManager& projectManager;
        PanelsManager& panelsManager;

        GameView gameView;
        SceneView sceneView;
        InspectorPanel inspectorPanel;
        SceneHierarchyPanel sceneHierarchyPanel;
        bool m_currentPlaying = false;

    public:
        ClientPanel(PanelsManager& panelsManager, Client& client, ProjectManager& projectManager);

        void onUpdate() override;

        void createToolBar();

        GameView& getGameView() {
            return gameView;
        }

        SceneView& getSceneView() {
            return sceneView;
        }

    private:
        void startRunningScene();

        void stopRunningScene();
    };
}
