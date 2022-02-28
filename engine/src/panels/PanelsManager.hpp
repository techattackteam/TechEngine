#pragma once

#include "Panel.hpp"
#include <imgui.h>
#include "../event/events/panels/OpenInspectorEvent.hpp"
#include "../event/events/panels/CloseInspectorEvent.hpp"
#include "RendererPanel.hpp"
#include "CustomPanel.hpp"
#include "SceneHierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "../event/events/panels/RegisterCustomPanel.hpp"

namespace Engine {
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

        void registerInspectorPanel(Engine::OpenInspectorEvent *event);

        void registerCustomPanel(Engine::RegisterCustomPanel *event);

        void unregisterPanel(Engine::CloseInspectorEvent *event);
    };
}


