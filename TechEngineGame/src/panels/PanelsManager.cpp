#include <iostream>
#include "PanelsManager.hpp"
#include "RendererPanel.hpp"
#include "InspectorPanel.hpp"
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include "../core/Core.hpp"

namespace TechEngine {
    PanelsManager::PanelsManager() {
        TechEngineCore::EventDispatcher::getInstance().subscribe(OpenInspectorEvent::eventType, [this](TechEngineCore::Event *event) {
            registerInspectorPanel((OpenInspectorEvent *) event);
        });

        TechEngineCore::EventDispatcher::getInstance().subscribe(CloseInspectorEvent::eventType, [this](TechEngineCore::Event *event) {
            unregisterPanel((CloseInspectorEvent *) event);
        });

        TechEngineCore::EventDispatcher::getInstance().subscribe(RegisterCustomPanel::eventType, [this](TechEngineCore::Event *event) {
            registerCustomPanel((RegisterCustomPanel *) event);
        });

        rendererPanel = new RendererPanel();
        sceneHierarchyPanel = new SceneHierarchyPanel();

#ifdef Engine_Editor
        initImGui(rendererPanel->getWindow());
#endif
    }

    void PanelsManager::update() {
#ifdef Engine_Editor
        rendererPanel->getWindow().getRenderer().beginImGuiFrame();
        sceneHierarchyPanel->onUpdate();
        for (Panel *panel: inspectorPanels) {
            panel->onUpdate();
        }

        for (CustomPanel *panel: customPanels) {
            panel->onUpdate();
        }
        rendererPanel->getWindow().getRenderer().ImGuiPipeline();
#endif
        if (rendererPanel != nullptr)
            rendererPanel->onUpdate();
#ifdef Engine_Editor
        ImGui::EndFrame();
#endif
    }


    void PanelsManager::registerInspectorPanel(OpenInspectorEvent *event) {
        if (event->getGameObject()->showInfoPanel) {
            inspectorPanels.emplace_back(new InspectorPanel(event->getGameObject()));
        }
    }

    void PanelsManager::registerCustomPanel(RegisterCustomPanel *event) {
        customPanels.emplace_back(event->getPanel());
        event->getPanel()->setupImGuiContext(imguiContext);
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
        for (Panel *panel: customPanels) {
            unregisterPanel(panel);
        }
        unregisterPanel(rendererPanel);
    }

    void PanelsManager::initImGui(Window &window) {
        imguiContext = ImGui::CreateContext();
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
