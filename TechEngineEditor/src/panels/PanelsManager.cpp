#include "PanelsManager.hpp"
#include "scene/SceneManager.hpp"
#include "script/ScriptEngine.hpp"
#include "event/events/appManagement/AppCloseRequestEvent.hpp"
#include "scene/SceneHelper.hpp"
#include "core/Logger.hpp"
#include "testGameObject/QuadMeshTest.hpp"
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <filesystem>
#include <commdlg.h>
#include <imgui_internal.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/emitter.h>
#include <fstream>
#include "project/ProjectManager.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseScrollEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"
#include "testGameObject/MainCamera.hpp"

namespace TechEngine {
    PanelsManager::PanelsManager(Window &window) : window(window), exportSettingsPanel((std::string &) "TEMPORARY") {
        instance = this;

    }

    void PanelsManager::init() {
        EventDispatcher::getInstance().subscribe(RegisterCustomPanel::eventType, [this](TechEngine::Event *event) {
            registerCustomPanel((RegisterCustomPanel *) event);
        });

        EventDispatcher::getInstance().subscribe(GameObjectDestroyEvent::eventType, [this](TechEngine::Event *event) {
            deselectGameObject(((GameObjectDestroyEvent *) event)->getGameObjectTag());
        });

        EventDispatcher::getInstance().subscribe(KeyPressedEvent::eventType, [this](TechEngine::Event *event) {
            OnKeyPressedEvent(((KeyPressedEvent *) event)->getKey());
        });

        EventDispatcher::getInstance().subscribe(KeyReleasedEvent::eventType, [this](TechEngine::Event *event) {
            OnKeyReleasedEvent(((KeyReleasedEvent *) event)->getKey());
        });

        EventDispatcher::getInstance().subscribe(MouseMoveEvent::eventType, [this](TechEngine::Event *event) {
            OnMouseMoveEvent(((MouseMoveEvent *) event)->getDelta());
        });

        EventDispatcher::getInstance().subscribe(MouseScrollEvent::eventType, [this](TechEngine::Event *event) {
            OnMouseScrollEvent(((MouseScrollEvent *) event)->getXOffset(), ((MouseScrollEvent *) event)->getYOffset());
        });
        contentBrowser.init();
        initImGui();
    }

    void PanelsManager::update() {
        beginImGuiFrame();
        for (CustomPanel *panel: customPanels) {
            panel->onUpdate();
        }
        sceneHierarchyPanel.onUpdate();
        inspectorPanel.onUpdate();
        sceneView.onUpdate();
        gameView.onUpdate();
        contentBrowser.onUpdate();
        exportSettingsPanel.onUpdate();
        endImGuiFrame();
    }


    void PanelsManager::registerCustomPanel(RegisterCustomPanel *event) {
        customPanels.emplace_back(event->getPanel());
        event->getPanel()->setupImGuiContext(imguiContext);
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
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
            }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
                openScene();
            }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                SceneManager::saveCurrentScene();
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
                } else {
                    exportSettingsPanel.setVisibility(true);
                }
            }
            if (ImGui::MenuItem("Exit")) {
                TechEngine::EventDispatcher::getInstance().dispatch(new AppCloseRequestEvent());
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
            } else {
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

    void PanelsManager::compileUserScripts() {
        if (!exists(ProjectManager::getUserProjectBuildPath())) {
            std::string command = "\"" + ProjectManager::getCmakePath().string() +
                                  " -G \"Visual Studio 17 2022\" -S " + "\"" + ProjectManager::getUserProjectScriptsPath().string() + "\"" +
                                  " -B " + "\"" + ProjectManager::getUserProjectBuildPath().string() + "\"" + "\"";
            std::system(command.c_str());
        }
        std::string command = "\"" + ProjectManager::getCmakePath().string() +
                              " --build " + "\"" + ProjectManager::getUserProjectBuildPath().string() + "\"" + " --target UserScripts --config Debug\"";
        std::system(command.c_str());
        TE_LOGGER_INFO("Build finished!");
    }

    void PanelsManager::startRunningScene() {
        SceneManager::saveSceneAsTemporarily(SceneManager::getActiveSceneName());
        ScriptEngine::getInstance()->init(ProjectManager::getUserScriptsDLLPath().string());
        m_currentPlaying = true;
    }

    void PanelsManager::stopRunningScene() {
        ScriptEngine::getInstance()->stop();
        SceneManager::loadSceneFromTemporarily(SceneManager::getActiveSceneName());
        for (GameObject *gameObject: Scene::getInstance().getGameObjects()) {
            if (gameObject->hasComponent<CameraComponent>() && gameObject->getComponent<CameraComponent>()->isMainCamera()) {
                SceneHelper::mainCamera = gameObject->getComponent<CameraComponent>();
            }
            if (gameObject->getTag() == gameObjectSelectedTag) {
                selectGameObject(gameObject->getTag());
            }
        }
        m_currentPlaying = false;
    }


    void PanelsManager::saveEngineSettings() {
/*        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene path" << YAML::Value << currentScenePath;
        out << YAML::EndSeq;
        out << YAML::EndMap;
        std::ofstream fout(ProjectManager::getEngineExportSettingsFile().string());
        fout << out.c_str();*/
        TE_LOGGER_INFO("EngineSettings saved");
    }

    void PanelsManager::openScene() {
        std::string filepath = openFileWindow("TechEngine Scene (*.scene)\0*.scene\0");
        std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);
        SceneManager::loadScene(filename);
    }

    PanelsManager &PanelsManager::getInstance() {
        return *instance;
    }

    void PanelsManager::selectGameObject(const std::string &tag) {
        gameObjectSelectedTag = tag;
    }

    void PanelsManager::deselectGameObject() {
        gameObjectSelectedTag = "";
    }

    void PanelsManager::deselectGameObject(std::string tag) {
        if (tag == gameObjectSelectedTag) {
            deselectGameObject();
        }
    }

    GameObject *PanelsManager::getSelectedGameObject() const {
        return Scene::getInstance().getGameObjectByTag(gameObjectSelectedTag);
    }

    void PanelsManager::OnKeyPressedEvent(Key &key) {
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
            case MOUSE_2: {
                mouse2 = true;
                break;
            }
            case MOUSE_3: {
                mouse3 = true;
                break;
            }
                TE_LOGGER_INFO("Key pressed event: {0}", key.getKeyName());
        }
    }

    void PanelsManager::OnKeyReleasedEvent(Key &key) {
        switch (key.getKeyCode()) {
            case MOUSE_2: {
                mouse2 = false;
                break;
            }
            case MOUSE_3: {
                mouse3 = false;
                break;
            }
        }
    }

    void PanelsManager::OnMouseScrollEvent(float xOffset, float yOffset) {
        const glm::mat4 inverted = glm::inverse(sceneView.getSceneCamera()->getComponent<CameraComponent>()->getViewMatrix());
        const glm::vec3 forward = normalize(glm::vec3(inverted[2]));
        if (yOffset == -1.0f) {
            sceneView.getSceneCamera()->getTransform().translate(forward);
        } else if (yOffset == 1.0f) {
            sceneView.getSceneCamera()->getTransform().translate(-forward);
        }
    }

    void PanelsManager::OnMouseMoveEvent(glm::vec2 delta) {
        if (m_currentPlaying) {
            return;
        }
        const glm::mat4 inverted = glm::inverse(sceneView.getSceneCamera()->getComponent<CameraComponent>()->getViewMatrix());
        const glm::vec3 right = normalize(glm::vec3(inverted[0]));
        const glm::vec3 up = normalize(glm::vec3(inverted[1]));
        if (mouse3) {
            const glm::vec3 move = -right * delta.x * 0.01f + up * delta.y * 0.01f;
            sceneView.getSceneCamera()->getTransform().translate(move);
        }
        if (mouse2) {
            const glm::vec3 rotate = glm::vec3(-delta.y * 0.5f, -delta.x * 0.5f, 0);
            sceneView.getSceneCamera()->getTransform().rotate(rotate);
        }
    }
}
