#include "PanelsManager.hpp"
#include "scene/SceneManager.hpp"
#include "script/ScriptEngine.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"
#include "scene/SceneHelper.hpp"
#include "core/Logger.hpp"
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <filesystem>
#include <commdlg.h>
#include <imgui_internal.h>
#include "project/ProjectManager.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "defaultGameObject/MainCamera.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    PanelsManager::PanelsManager(Window&window, SceneManager&sceneManager,
                                 ProjectManager&projectManager,
                                 PhysicsEngine&physicsEngine,
                                 TextureManager&textureManager,
                                 MaterialManager&materialManager) : window(window),
                                                                    sceneManager(sceneManager),
                                                                    projectManager(projectManager),
                                                                    physicsEngine(physicsEngine),

                                                                    gameView(window.getRenderer(), sceneManager.getScene()),
                                                                    contentBrowser(*this, projectManager, sceneManager, materialManager),
                                                                    exportSettingsPanel(*this, projectManager, sceneManager, window.getRenderer().getShadersManager()),
                                                                    sceneHierarchyPanel(sceneManager.getScene(), materialManager),
                                                                    sceneView(window.getRenderer(), sceneManager.getScene(), sceneHierarchyPanel.getSelectedGO()),
                                                                    inspectorPanel(sceneHierarchyPanel.getSelectedGO(), materialManager, physicsEngine),
                                                                    materialEditor(window.getRenderer(), textureManager, materialManager, sceneManager.getScene()) {
    }

    void PanelsManager::init() {
        EventDispatcher::getInstance().subscribe(RegisterCustomPanel::eventType, [this](TechEngine::Event *event) {
            registerCustomPanel((RegisterCustomPanel *) event);
        });

        EventDispatcher::getInstance().subscribe(GameObjectDestroyEvent::eventType, [this](TechEngine::Event *event) {
            sceneHierarchyPanel.deselectGO(sceneManager.getScene().getGameObjectByTag(((GameObjectDestroyEvent *) event)->getGameObjectTag()));
        });

        EventDispatcher::getInstance().subscribe(KeyPressedEvent::eventType, [this](TechEngine::Event *event) {
            OnKeyPressedEvent(((KeyPressedEvent *) event)->getKey());
        });


        contentBrowser.init();
        materialEditor.init();
        initImGui();
    }

    void PanelsManager::update() {
        beginImGuiFrame();
        for (CustomPanel* panel: customPanels) {
            panel->onUpdate();
        }
        sceneHierarchyPanel.onUpdate();
        inspectorPanel.onUpdate();
        sceneView.onUpdate();
        gameView.onUpdate();
        contentBrowser.onUpdate();
        exportSettingsPanel.onUpdate();
        materialEditor.onUpdate();
        endImGuiFrame();
    }

    void PanelsManager::registerCustomPanel(RegisterCustomPanel* event) {
        customPanels.emplace_back(event->getPanel());
        event->getPanel()->setupImGuiContext(imguiContext);
    }

    void PanelsManager::initImGui() {
        imguiContext = ImGui::CreateContext();
        ImGuiIO&io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigViewportsNoAutoMerge = true;

        ImGui::StyleColorsDark();

        ImGuiStyle&style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
        }

        setColorTheme();

        ImGui_ImplGlfw_InitForOpenGL(window.getHandler(), true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void PanelsManager::setColorTheme() {
        ImGuiStyle&style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Modify the blue color to make it more grayish
        colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f); // Gray text
        colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f); // Dark gray text (disabled)
        colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // Dark gray window background
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.30f); // Light gray border
        colors[ImGuiCol_BorderShadow] = ImVec4(0.10f, 0.10f, 0.10f, 0.20f); // Dark gray border shadow
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Gray frame background
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f); // Light blue frame background (hovered)
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f); // Light blue frame background (active)
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // Dark gray title background
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // Dark gray title background (active)
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f); // Dark gray title background (collapsed)
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Gray menu bar background
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // Dark gray scrollbar background
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f); // Light blue scrollbar grab
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f); // Light blue scrollbar grab (hovered)
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue scrollbar grab (active)
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue check mark
        colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f); // Blue slider grab
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue slider grab (active)
        colors[ImGuiCol_Button] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f); // Lighter gray button
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f); // Light blue button (hovered)
        colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue button (active)
        colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f); // Light blue header
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f); // Light blue header (hovered)
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue header (active)
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border]; // Light gray separator
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f); // Light blue separator (hovered)
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue separator (active)
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f); // Light blue resize grip
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f); // Light blue resize grip (hovered)
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue resize grip (active)
        colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f); // Gray tab
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f); // Light blue tab (hovered)
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f); // Dark gray tab (active)
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f); // Gray tab (unfocused)
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f); // Gray tab (unfocused, active)
        colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f); // Light blue docking preview
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Gray docking empty background

        style.WindowRounding = 2.0f;
        style.FrameRounding = 2.0f;
        style.FrameBorderSize = 1.0f;
        style.PopupRounding = 2.0f;
        style.ChildRounding = 2.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 2.0f;
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
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
        ImGuiViewport* viewport = ImGui::GetMainViewport();
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
        ImGuiIO&io = ImGui::GetIO();
        ImGuiStyle&style = ImGui::GetStyle();
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
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
            }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
            }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                projectManager.saveProject();
            }
            if (ImGui::MenuItem("Build")) {
                if (m_currentPlaying) {
                    stopRunningScene();
                    m_currentPlaying = false;
                }

                compileUserScripts();
            }

            if (ImGui::MenuItem("Export")) {
                if (exportSettingsPanel.isVisible()) {
                    exportSettingsPanel.setVisibility(false);
                }
                else {
                    exportSettingsPanel.setVisibility(true);
                }
            }
            if (ImGui::MenuItem("Exit")) {
                EventDispatcher::getInstance().dispatch(new AppCloseRequestEvent());
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    void PanelsManager::createToolBar() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto&colors = ImGui::GetStyle().Colors;
        const auto&buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto&buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        float size = ImGui::GetWindowWidth() / 7;
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() / 2 - 10);
        if (ImGui::Button(sceneView.getGuizmoMode() == ImGuizmo::MODE::WORLD ? "World" : "Local", ImVec2(size, 0))) {
            sceneView.changeGuizmoMode(sceneView.getGuizmoMode() == ImGuizmo::MODE::WORLD ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x / 2) - (size / 2));
        if (ImGui::Button(m_currentPlaying == true ? "Stop" : "Play", ImVec2(size, 0))) {
            if (!m_currentPlaying) {
                startRunningScene();
            }
            else {
                stopRunningScene();
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
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    std::string PanelsManager::openFileWindow(const char* filter) {
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

    std::string PanelsManager::saveFile(const char* filter) {
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

    void PanelsManager::compileUserScripts() {
        if (!exists(projectManager.getScriptsBuildPath()) || std::filesystem::is_empty(projectManager.getScriptsBuildPath())) {
            std::string command = "\"" + projectManager.getCmakePath().string() +
                                  " -G \"Visual Studio 17 2022\" -S " + "\"" + projectManager.getCmakeListPath().string() + "\"" +
                                  " -B " + "\"" + projectManager.getScriptsBuildPath().string() + "\"" + "\"";
            std::system(command.c_str());
        }
        std::string command = "\"" + projectManager.getCmakePath().string() +
                              " --build " + "\"" + projectManager.getScriptsBuildPath().string() + "\"" + " --target UserScripts --config Debug\"";
        std::system(command.c_str());
        TE_LOGGER_INFO("Build finished!");
    }

    void PanelsManager::startRunningScene() {
        sceneManager.saveSceneAsTemporarily(sceneManager.getActiveSceneName());
        ScriptEngine::getInstance()->init(projectManager.getScriptsDLLPath().string());
        ScriptEngine::getInstance()->onStart();
        physicsEngine.start();
        m_currentPlaying = true;
    }

    void PanelsManager::stopRunningScene() {
        ScriptEngine::getInstance()->stop();
        physicsEngine.stop();
        sceneManager.loadSceneFromTemporarily(sceneManager.getActiveSceneName());
        sceneHierarchyPanel.getSelectedGO().clear();
        for (GameObject* gameObject: sceneManager.getScene().getGameObjects()) {
            if (gameObject->hasComponent<CameraComponent>() && gameObject->getComponent<CameraComponent>()->isMainCamera()) {
                SceneHelper::mainCamera = gameObject->getComponent<CameraComponent>();
            }
            if (std::find(getSelectedGameObjects().begin(), getSelectedGameObjects().end(), gameObject) != getSelectedGameObjects().end()) {
                sceneHierarchyPanel.selectGO(gameObject);
            }
        }
        m_currentPlaying = false;
    }

    std::vector<GameObject *>& PanelsManager::getSelectedGameObjects() {
        return sceneHierarchyPanel.getSelectedGO();
    }

    void PanelsManager::OnKeyPressedEvent(Key&key) {
        switch (key.getKeyCode()) {
            case Q: {
                if (!ImGuizmo::IsUsing())
                    sceneView.changeGuizmoOperation(-1);
                break;
            }
            case T: {
                if (!ImGuizmo::IsUsing())
                    sceneView.changeGuizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
                break;
            }
            case R: {
                if (!ImGuizmo::IsUsing())
                    sceneView.changeGuizmoOperation(ImGuizmo::OPERATION::ROTATE);
                break;
            }
            case S: {
                if (!ImGuizmo::IsUsing())
                    sceneView.changeGuizmoOperation(ImGuizmo::OPERATION::SCALE);
                break;
            }
                TE_LOGGER_INFO("Key pressed event: {0}", key.getKeyName());
        }
    }


    void PanelsManager::openMaterialEditor(const std::string&materialName, const std::string&filepath) {
        materialEditor.open(materialName, filepath);
    }
}
