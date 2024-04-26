#pragma once

#include "Panel.hpp"
#include "GameView.hpp"
#include "InspectorPanel.hpp"
#include "SceneView.hpp"
#include "SceneHierarchyPanel.hpp"
#include "core/Client.hpp"

namespace TechEngine {
    class ClientPanel : public Panel {
    private:
        Client& client;
        GameView gameView;
        SceneView sceneView;
        InspectorPanel inspectorPanel;
        SceneHierarchyPanel sceneHierarchyPanel;
        bool m_currentPlaying = false;

    public:
        ClientPanel(Client& client);

        void onUpdate() override;

        void createToolBar();

        GameView& getGameView() {
            return gameView;
        }

        SceneView& getSceneView() {
            return sceneView;
        }

    private:
    };
}
