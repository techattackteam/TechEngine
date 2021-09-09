#include <iostream>
#include "PanelsManager.hpp"
#include "RendererPanel.hpp"

namespace Engine {
    PanelsManager::PanelsManager() {
        EventDispatcher::getInstance().subscribe(PanelCreateEvent::eventType, [this](Event *event) {
            registerPanel((PanelCreateEvent *) event);
        });
    }

    void PanelsManager::update() {
        if (!imguiPanels.empty()) {
            rendererPanel->getWindow().getRenderer().beginImGuiFrame();
            for (Panel *panel: imguiPanels) {
                panel->onUpdate();
            }
            rendererPanel->getWindow().getRenderer().ImGuiPipeline();
        }
        rendererPanel->onUpdate();
    }

    void PanelsManager::registerPanel(PanelCreateEvent *event) {
        if (event->getPanel()->isMainPanel()) {
            rendererPanel = (RendererPanel *) event->getPanel();
            initImGui(((RendererPanel *) event->getPanel())->getWindow());
        } else {
            imguiPanels.emplace_back((ImGuiPanel *) event->getPanel());
        }
    }

    void PanelsManager::unregisterPanel(Panel *panel) {
        auto index = std::find(imguiPanels.begin(), imguiPanels.end(), panel);
        if (index != imguiPanels.end()) {
            imguiPanels.erase(index);
        }
        delete (panel);
    }

    void PanelsManager::unregisterAllPanels() {
        for (Panel *panel: imguiPanels) {
            unregisterPanel(panel);
        }
        unregisterPanel(rendererPanel);
    }

    void PanelsManager::initImGui(Window &window) {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigViewportsNoAutoMerge = true;

        ImGui::StyleColorsDark();

        ImGuiStyle &style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        ImGui_ImplGlfw_InitForOpenGL(window.getHandler(), true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }
}
