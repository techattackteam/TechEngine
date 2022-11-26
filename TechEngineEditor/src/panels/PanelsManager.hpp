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
#include "events/panels/RegisterCustomPanel.hpp"
#include "ExportSettingsPanel.hpp"

namespace TechEngine {
    class PanelsManager {
    private:
        std::vector<CustomPanel *> customPanels;
        ImGuiContext *imguiContext;
        Window &window;
        RendererPanel rendererPanel{window.getRenderer()};
        InspectorPanel inspectorPanel;
        //SettingsPanel settingsPanel{};
        ProjectBrowserPanel contentBrowser;
        SceneHierarchyPanel sceneHierarchyPanel;
        ExportSettingsPanel exportSettingsPanel;

        //Temporary
        const std::string currentDirectory = std::filesystem::current_path().string();
        const std::string projectDirectory = currentDirectory + "/project";
        const std::string buildDirectory = currentDirectory + "/build";
        const std::string cmakeProjectDirectory = projectDirectory + "/scripts";

        std::string currentScenePath;

        bool m_currentPlaying = false;
    public:

        PanelsManager(Window &window);

        void update();

        static void compileUserScripts(const std::filesystem::path &projectPath, const std::filesystem::path &dllTargetPath);

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


