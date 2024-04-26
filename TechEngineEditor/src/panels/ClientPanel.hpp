#pragma once

#include "Panel.hpp"
#include "GameView.hpp"
#include "InspectorPanel.hpp"
#include "SceneView.hpp"
#include "SceneHierarchyPanel.hpp"
#include "core/Client.hpp"

namespace TechEngine {
    enum CompileMode;

    class ClientPanel : public Panel {
    private:
        Client& client;
        ProjectManager& projectManager;
        GameView gameView;
        SceneView sceneView;
        InspectorPanel inspectorPanel;
        SceneHierarchyPanel sceneHierarchyPanel;
        bool m_currentPlaying = false;

    public:
        ClientPanel(Client& client, ProjectManager& projectManager);

        void onUpdate() override;

        void createToolBar();

        GameView& getGameView() {
            return gameView;
        }

        SceneView& getSceneView() {
            return sceneView;
        }

        void compileClientUserScripts(CompileMode compileMode);

    private:
        void startRunningScene();

        void stopRunningScene();
    };
}
