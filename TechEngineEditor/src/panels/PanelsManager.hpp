#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NOMINMAX

#include <windows.h>
#include "core/Window.hpp"
#include "SceneHierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "SceneView.hpp"
#include "SettingsPanel.hpp"
#include "ContentBrowserPanel.hpp"
#include "ExportSettingsPanel.hpp"
#include "events/panels/RegisterCustomPanel.hpp"
#include "core/Key.hpp"
#include "GameView.hpp"
#include "core/SceneCamera.hpp"
#include "MaterialEditor.hpp"
#include "project/ProjectManager.hpp"

namespace TechEngine {
    class PanelsManager {
    private:
        std::vector<CustomPanel *> customPanels;
        ImGuiContext *imguiContext;
        Window &window;
        ProjectManager &projectManager;
        SceneManager &sceneManager;
        PhysicsEngine &physicsEngine;


        GameView gameView;
        //SettingsPanel settingsPanel{};
        ContentBrowserPanel contentBrowser;
        ExportSettingsPanel exportSettingsPanel;
        SceneHierarchyPanel sceneHierarchyPanel;
        SceneView sceneView;
        InspectorPanel inspectorPanel;
        MaterialEditor materialEditor;
        bool m_currentPlaying = false;

    public:

        PanelsManager(Window &window, SceneManager &sceneManager,
                      ProjectManager &projectManager,
                      PhysicsEngine &physicsEngine,
                      TextureManager &textureManager,
                      MaterialManager &materialManager);

        void init();

        void update();

        void compileUserScripts();

        std::vector<GameObject *> &getSelectedGameObjects();

        void openMaterialEditor(const std::string &materialName, const std::string &filepath);

    private:
        void initImGui();

        void registerCustomPanel(TechEngine::RegisterCustomPanel *event);

        void beginImGuiFrame();

        void createDockSpace();

        void createMenuBar();

        void createToolBar();

        void endImGuiFrame();

        std::string openFileWindow(const char *filter);

        std::string saveFile(const char *filter);

        void startRunningScene();

        void stopRunningScene();

        void OnKeyPressedEvent(Key &key);

        void setColorTheme();
    };
}


