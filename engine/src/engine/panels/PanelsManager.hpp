#pragma once

#include "Panel.hpp"
#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"
#include "../event/events/panels/PanelCreateEvent.hpp"
#include "RendererPanel.hpp"
#include "ImGuiPanel.hpp"

namespace Engine {
    class PanelsManager {
    private:
        std::vector<ImGuiPanel *> imguiPanels;
        RendererPanel *rendererPanel;
    public:
        PanelsManager();

        void update();

        void registerPanel(PanelCreateEvent *);

        void unregisterPanel(Panel *panel);

        void unregisterAllPanels();

    private:
        static void initImGui(Window &window);

    };
}


