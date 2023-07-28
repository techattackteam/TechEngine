#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include "core/Window.hpp"
#include "SceneHierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "RendererPanel.hpp"
#include "SettingsPanel.hpp"
#include "ProjectBrowserPanel.hpp"
#include "ExportSettingsPanel.hpp"
#include "events/panels/RegisterCustomPanel.hpp"

namespace TechEngine {
    class PanelsManager {
    private:
        inline static PanelsManager *instance;
        std::vector<CustomPanel *> customPanels;
        ImGuiContext *imguiContext;
        Window &window;
        RendererPanel rendererPanel{window.getRenderer()};
        InspectorPanel inspectorPanel;
        //SettingsPanel settingsPanel{};
        ProjectBrowserPanel contentBrowser;
        SceneHierarchyPanel sceneHierarchyPanel;
        ExportSettingsPanel exportSettingsPanel;

        std::string currentScenePath;
        bool m_currentPlaying = false;

        GameObject *selectedGameObject = nullptr;
    public:

        PanelsManager(Window &window);

        void update();

        static void compileUserScripts();

        GameObject *getSelectedGameObject() const;

        static PanelsManager &getInstance();

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

        void onCloseAppEvent();

        void saveScene();

        void saveSceneAs();

        void openScene();

        void openSceneOnStartup();

        void saveEngineSettings();
    };
}


