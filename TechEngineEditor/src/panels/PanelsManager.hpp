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

namespace TechEngine {
    class PanelsManager {
    private:
        inline static PanelsManager *instance;
        std::vector<CustomPanel *> customPanels;
        ImGuiContext *imguiContext;
        Window &window;
        SceneView sceneView{window.getRenderer()};
        GameView gameView{window.getRenderer()};
        InspectorPanel inspectorPanel;
        //SettingsPanel settingsPanel{};
        ContentBrowserPanel contentBrowser;
        SceneHierarchyPanel sceneHierarchyPanel;
        ExportSettingsPanel exportSettingsPanel;
        MaterialEditor materialEditor{window.getRenderer()};
        bool m_currentPlaying = false;

        std::string gameObjectSelectedTag;

        bool mouse2 = false;
        bool mouse3 = false;
    public:

        PanelsManager(Window &window);

        void init();

        void update();

        static void compileUserScripts();

        GameObject *getSelectedGameObject() const;

        static PanelsManager &getInstance();

        void selectGameObject(const std::string &gameObjectTag);

        void deselectGameObject();

        void deselectGameObject(std::string gameObject);

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

        void OnMouseScrollEvent(float xOffset, float yOffset);

        void OnMouseMoveEvent(glm::vec2 delta);

        void OnKeyReleasedEvent(Key &key);

        void setColorTheme();
    };
}


