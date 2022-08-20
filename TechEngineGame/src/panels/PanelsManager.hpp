#pragma once

#include "Panel.hpp"
#include <imgui.h>
#include "RendererPanel.hpp"
#include "CustomPanel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "../events/panels/OpenInspectorEvent.hpp"
#include "../events/panels/CloseInspectorEvent.hpp"
#include "../events/panels/RegisterCustomPanel.hpp"


namespace TechEngine {
    class PanelsManager {
    private:
        std::vector<InspectorPanel *> inspectorPanels;
        std::vector<CustomPanel *> customPanels;
        SceneHierarchyPanel *sceneHierarchyPanel;
        ImGuiContext *imguiContext;
    public:
        RendererPanel *rendererPanel;

        PanelsManager();

        void update();

        void unregisterPanel(Panel *panel);

        void unregisterAllPanels();

    private:
        void initImGui(Window &window);

        void registerInspectorPanel(TechEngine::OpenInspectorEvent *event);

        void registerCustomPanel(TechEngine::RegisterCustomPanel *event);

        void unregisterPanel(TechEngine::CloseInspectorEvent *event);
    };
}


