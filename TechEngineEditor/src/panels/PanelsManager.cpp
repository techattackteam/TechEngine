#include "PanelsManager.hpp"
#include "scene/SceneSerializer.hpp"
#include "script/ScriptEngine.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <filesystem>
#include <commdlg.h>
#include <imgui_internal.h>

namespace TechEngine {
    PanelsManager::PanelsManager(Window &window) : window(window), exportSettingsPanel(currentDirectory, projectDirectory, buildDirectory, cmakeProjectDirectory, currentScenePath) {
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
        exportSettingsPanel.onUpdate();
        endImGuiFrame();
    }


    void PanelsManager::registerCustomPanel(RegisterCustomPanel *event) {
        customPanels.emplace_back(event->getPanel());
        event->getPanel()->setupImGuiContext(imguiContext);
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

        createDockSpace();
        createMenuBar();
        createToolBar();

        ImGui::End();
    }

    void PanelsManager::createDockSpace() {
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
        style.WindowMinSize.x = minWinSizeX;
    }

    void PanelsManager::createMenuBar() {
        ImGui::BeginMenuBar();
        if (ImGui::BeginMenu("File")) {
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
                    SceneSerializer::serialize("project/scenes/Scene1.scene");
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
                if (m_currentPlaying) {
                    stopRunningScene();
                    m_currentPlaying = false;
                }
                std::string projectDirectory = std::filesystem::current_path().string() + "/project";
                std::string buildDirectory = std::filesystem::current_path().string() + "/build";
                for (const auto &entry: std::filesystem::directory_iterator(buildDirectory))
                    std::filesystem::remove_all(entry.path());
                std::filesystem::copy(projectDirectory, buildDirectory, std::filesystem::copy_options::recursive);
                compileUserScripts(projectDirectory, std::filesystem::current_path());
            }

            if (ImGui::MenuItem("Export")) {
                if (exportSettingsPanel.isVisible()) {
                    exportSettingsPanel.setVisibility(false);
                } else {
                    exportSettingsPanel.setVisibility(true);
                }
            }
            if (ImGui::MenuItem("Exit")) {
                TechEngineCore::EventDispatcher::getInstance().dispatch(new AppCloseRequestEvent());
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    void PanelsManager::createToolBar() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto &colors = ImGui::GetStyle().Colors;
        const auto &buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto &buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        float size = ImGui::GetWindowWidth() / 5;
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x / 2) - (size / 2));
        if (ImGui::Button(m_currentPlaying == true ? "Stop" : "Play", ImVec2(size, 0))) {
            if (!m_currentPlaying) {
                startRunningScene();
                m_currentPlaying = true;
            } else {
                stopRunningScene();
                m_currentPlaying = false;
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

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

    void PanelsManager::compileUserScripts(const std::filesystem::path &cmakeProjectDirectory, const std::filesystem::path &dllTargetPath) {
        std::string dllPath = cmakeProjectDirectory.string() + "/runtime/UserProject.dll";

        std::string command = "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars32.bat\" "
                              "&& cmake --build " + cmakeProjectDirectory.string() + " --target UserProject -j 12";

        std::system(command.c_str());
    }

    void PanelsManager::startRunningScene() {
        SceneSerializer::serialize(projectDirectory + "/scenes/SceneSaveTemporary.scene");
        ScriptEngine::getInstance()->init(cmakeProjectDirectory + "/runtime/UserProject.dll");
    }

    void PanelsManager::stopRunningScene() {
        ScriptEngine::getInstance()->stop();
        SceneSerializer::deserialize(projectDirectory + "/scenes/SceneSaveTemporary.scene");
        std::filesystem::remove(projectDirectory + "/scenes/SceneSaveTemporary.scene");
    }


}
