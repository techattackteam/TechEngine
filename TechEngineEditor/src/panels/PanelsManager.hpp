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

namespace TechEngine {
    class PanelsManager {
    private:
        std::vector<CustomPanel *> customPanels;
        ImGuiContext *imguiContext;
        RendererPanel rendererPanel{};
        InspectorPanel inspectorPanel{};
        //SettingsPanel settingsPanel{};
        ProjectBrowserPanel contentBrowser{};
        SceneHierarchyPanel sceneHierarchyPanel{};
        Window &window;

        std::filesystem::path currentScenePath;

        HINSTANCE m_userCustomDll = nullptr;
    public:

        PanelsManager(Window &window);

        void update();

        void unregisterPanel(Panel *panel);

        void unregisterAllPanels();

    private:
        void initImGui();

        void registerCustomPanel(TechEngine::RegisterCustomPanel *event);

        void beginImGuiFrame();

        void endImGuiFrame();

        std::string openFileWindow(const char *filter);

        std::string saveFile(const char *filter);

        void compileUserScripts(const std::filesystem::path &projectPath, const std::filesystem::path &dllTargetPath);
    };
}


