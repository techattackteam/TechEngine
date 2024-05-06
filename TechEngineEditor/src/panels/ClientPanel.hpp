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
        friend class PanelsManager;

    public:
        ClientPanel(Client& client, EventDispatcher& eventDispatcher, PanelsManager& panelsManager, ProjectManager& projectManager);

        void onUpdate() override;

        void createToolBar();

        GameView& getGameView() {
            return gameView;
        }

        SceneView& getSceneView() {
            return sceneView;
        }

        const bool& isRunning() const {
            return m_currentPlaying;
        }

    private:
        void startRunningScene();

        void stopRunningScene();
    };
}
