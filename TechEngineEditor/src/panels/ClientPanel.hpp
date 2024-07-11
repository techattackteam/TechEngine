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
    class ClientPanel : public Panel {
    private:
        Client& client;
        SystemsRegistry& editorRegistry;
        SystemsRegistry& appRegistry;
        PanelsManager& panelsManager;

        GameView gameView;
        SceneView sceneView;
        InspectorPanel inspectorPanel;
        SceneHierarchyPanel sceneHierarchyPanel;
        bool m_currentPlaying = false;
        friend class PanelsManager;

    public:
        ClientPanel(Client& client, SystemsRegistry& editorRegistry, SystemsRegistry& appRegistry, PanelsManager& panelsManager);

        void init() override;

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
