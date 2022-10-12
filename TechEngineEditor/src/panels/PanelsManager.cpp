#include "PanelsManager.hpp"
#include "scene/Scene.hpp"
#include "testGameObject/QuadMeshTest.hpp"
#include "scene/SceneSerializer.hpp"
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <filesystem>
#include <commdlg.h>

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
        contentBrowser.onUpdate();
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
        style.WindowMinSize.x = 170.0f;
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
                    std::string filepath = openFileWindow("TechEngine Scene (*.scene)\0*.scene\0");
                    SceneSerializer::deserialize(filepath);
                }

                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    if (currentScenePath.empty()) {
                        std::string filepath = saveFile("TechEngine Scene (*.scene)\0*.scene\0");
                        if (!filepath.empty()) {
                            SceneSerializer::serialize(filepath);
                            currentScenePath = filepath;
                        }
                    } else {
                        SceneSerializer::serialize("project\\scenes\\Scene1.scene");
                    }
                }

                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                    std::string filepath = saveFile("TechEngine Scene (*.scene)\0*.scene\0");
                    if (!filepath.empty()) {
                        SceneSerializer::serialize(filepath);
                        currentScenePath = filepath;
                    }
                }

                if (ImGui::MenuItem("Build")) {
                    system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat\""
                           " && cmake --build ../../cmake-build-debug --target TechEngineScript");
                    std::string projectDirectory = std::filesystem::current_path().string() + "/project";
                    std::string buildDirectory = std::filesystem::current_path().string() + "/build";
                    for (const auto &entry: std::filesystem::directory_iterator(buildDirectory))
                        std::filesystem::remove_all(entry.path());
                    std::filesystem::copy(projectDirectory, buildDirectory, std::filesystem::copy_options::recursive);
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

    std::string PanelsManager::openFileWindow(const char *filter) {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        CHAR currentDir[256] = {0};
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        //ofn.hwndOwner = glfwGetWin32Window((GLFWwindow *) window.getHandler());
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        if (GetCurrentDirectoryA(256, currentDir))
            ofn.lpstrInitialDir = currentDir;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return std::string();
    }

    std::string PanelsManager::saveFile(const char *filter) {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        CHAR currentDir[256] = {0};
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        if (GetCurrentDirectoryA(256, currentDir))
            ofn.lpstrInitialDir = currentDir;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        // Sets the default extension by extracting it from the filter
        ofn.lpstrDefExt = strchr(filter, '\0') + 1;

        if (GetSaveFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return std::string();
    }
}
