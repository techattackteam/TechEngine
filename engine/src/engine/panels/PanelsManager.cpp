#include <iostream>
#include "PanelsManager.hpp"
#include "RendererPanel.hpp"
#include "InspectorPanel.hpp"
#include "../event/events/panels/RegisterCustomPanel.hpp"


namespace Engine {
    PanelsManager::PanelsManager() {
        EventDispatcher::getInstance().subscribe(OpenInspectorEvent::eventType, [this](Event *event) {
            registerInspectorPanel((OpenInspectorEvent *) event);
        });

        EventDispatcher::getInstance().subscribe(CloseInspectorEvent::eventType, [this](Event *event) {
            unregisterPanel((CloseInspectorEvent *) event);
        });

        EventDispatcher::getInstance().subscribe(RegisterCustomPanel::eventType, [this](Event *event) {
            registerCustomPanel((RegisterCustomPanel *) event);
        });

        rendererPanel = new RendererPanel();
        sceneHierarchyPanel = new SceneHierarchyPanel();

        initImGui(rendererPanel->getWindow());
    }

    void PanelsManager::update() {
        rendererPanel->getWindow().getRenderer().beginImGuiFrame();
        sceneHierarchyPanel->onUpdate();
        for (Panel *panel: inspectorPanels) {
            panel->onUpdate();
        }

        for (CustomPanel *panel: customPanels) {
            panel->onUpdate();
        }

        rendererPanel->getWindow().getRenderer().ImGuiPipeline();
        if (rendererPanel != nullptr)
            rendererPanel->onUpdate();
    }


    void PanelsManager::registerInspectorPanel(OpenInspectorEvent *event) {
        if (event->getGameObject()->showInfoPanel) {
            inspectorPanels.emplace_back(new InspectorPanel(event->getGameObject()));
        }
    }

    void PanelsManager::registerCustomPanel(RegisterCustomPanel *event) {
        customPanels.emplace_back(event->getPanel());
    }

    void PanelsManager::unregisterPanel(CloseInspectorEvent *event) {
        for (InspectorPanel *panel: inspectorPanels) {
            if (panel->getGameObject()->getName() == event->getGameObject()->getName()) {
                unregisterPanel(panel);
                break;
            }
        }
    }

    void PanelsManager::unregisterPanel(Panel *panel) {
        auto index = std::find(inspectorPanels.begin(), inspectorPanels.end(), panel);
        if (index != inspectorPanels.end()) {
            inspectorPanels.erase(index);
        }
        delete (panel);
    }

    void PanelsManager::unregisterAllPanels() {
        for (Panel *panel: inspectorPanels) {
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
