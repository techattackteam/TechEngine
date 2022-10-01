#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "core/Window.hpp"
#include "SceneHierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "RendererPanel.hpp"
#include "SettingsPanel.hpp"
#include "events/panels/OpenInspectorEvent.hpp"
#include "events/panels/RegisterCustomPanel.hpp"
#include "events/panels/CloseInspectorEvent.hpp"

namespace TechEngine {
    class PanelsManager {
    private:
        std::vector<CustomPanel *> customPanels;
        ImGuiContext *imguiContext;
        RendererPanel rendererPanel{};
        //InspectorPanel inspectorPanel{};
        //SettingsPanel settingsPanel{};
        SceneHierarchyPanel sceneHierarchyPanel{};
        Window &window;
    public:

        PanelsManager(Window &window);

        void update();

        void unregisterPanel(Panel *panel);

        void unregisterAllPanels();

    private:
        void initImGui();

        void registerInspectorPanel(TechEngine::OpenInspectorEvent *event);

        void registerCustomPanel(TechEngine::RegisterCustomPanel *event);

        void unregisterPanel(TechEngine::CloseInspectorEvent *event);

        void beginImGuiFrame();

        void endImGuiFrame();

        void onWindowResizeEvent(WindowResizeEvent *event);
    };
}


