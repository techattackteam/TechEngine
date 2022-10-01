#include "PanelsManager.hpp"
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

namespace TechEngine {
    PanelsManager::PanelsManager(Window &window) : window(window) {
        TechEngineCore::EventDispatcher::getInstance().subscribe(OpenInspectorEvent::eventType, [this](TechEngineCore::Event *event) {
            registerInspectorPanel((OpenInspectorEvent *) event);
        });

        TechEngineCore::EventDispatcher::getInstance().subscribe(CloseInspectorEvent::eventType, [this](TechEngineCore::Event *event) {
            unregisterPanel((CloseInspectorEvent *) event);
        });

        TechEngineCore::EventDispatcher::getInstance().subscribe(RegisterCustomPanel::eventType, [this](TechEngineCore::Event *event) {
            registerCustomPanel((RegisterCustomPanel *) event);
        });

        initImGui();
    }

    void PanelsManager::update() {
        beginImGuiFrame();
        for (CustomPanel *panel: customPanels) {
            panel->onUpdate();
        }
        sceneHierarchyPanel.onUpdate();
        inspectorPanel.onUpdate();
        rendererPanel.onUpdate();

        endImGuiFrame();
    }


    void PanelsManager::registerInspectorPanel(OpenInspectorEvent *event) {
        if (event->getGameObject()->showInfoPanel) {
            //inspectorPanels.emplace_back(new InspectorPanel(event->getGameObject()));
        }
    }

    void PanelsManager::registerCustomPanel(RegisterCustomPanel *event) {
        customPanels.emplace_back(event->getPanel());
        event->getPanel()->setupImGuiContext(imguiContext);
    }

    void PanelsManager::unregisterPanel(CloseInspectorEvent *event) {
/*      for (InspectorPanel *panel: inspectorPanels) {
            if (panel->getGameObject()->getName() == event->getGameObject()->getName()) {
                unregisterPanel(panel);
                break;
            }
        }*/
    }

    void PanelsManager::unregisterPanel(Panel *panel) {
/*        auto index = std::find(inspectorPanels.begin(), inspectorPanels.end(), panel);
        if (index != inspectorPanels.end()) {
            inspectorPanels.erase(index);
        }
        delete (panel);*/
    }

    void PanelsManager::unregisterAllPanels() {
/*        for (Panel *panel: inspectorPanels) {
            unregisterPanel(panel);
        }
        for (Panel *panel: customPanels) {
            unregisterPanel(panel);
        }
        unregisterPanel(rendererPanel);*/
    }

    void PanelsManager::initImGui() {
        imguiContext = ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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

    void PanelsManager::beginImGuiFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Note: Switch this to true to enable dockspace
        static bool dockspaceOpen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO &io = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);1
                if (ImGui::MenuItem("New", "Ctrl+N")) {
                }

                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                }

                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                }

                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {

                }

                if (ImGui::MenuItem("Exit")) {}
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        style.WindowMinSize.x = minWinSizeX;
        ImGui::End();
    }

    void PanelsManager::endImGuiFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGui::EndFrame();
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}
