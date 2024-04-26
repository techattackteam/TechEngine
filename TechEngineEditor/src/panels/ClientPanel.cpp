#include "ClientPanel.hpp"

#include "PanelsManager.hpp"
#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    ClientPanel::ClientPanel(Client& client, ProjectManager& projectManager) : client(client),
                                                                               projectManager(projectManager),
                                                                               gameView(client.renderer, client.sceneManager.getScene()),
                                                                               inspectorPanel("Client Inspector", sceneHierarchyPanel.getSelectedGO(), client.materialManager, client.physicsEngine),
                                                                               sceneView("Client Scene", 1, client.renderer, client.sceneManager.getScene(), client.physicsEngine, sceneHierarchyPanel.getSelectedGO()),
                                                                               sceneHierarchyPanel("Client Scene Hierarchy", client.sceneManager.getScene(), client.materialManager),
                                                                               Panel("ClientPanel") {
    }

    void ClientPanel::onUpdate() {
        // Create a dockspace in the window
        ImGuiID dockspaceId = ImGui::GetID("ClientDockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // Create an ImGuiWindowClass and set the DockNodeFlagsOverrideSet field
        ImGuiWindowClass windowClass;
        windowClass.ClassId = dockspaceId;
        windowClass.DockingAllowUnclassed = false;
        ImGui::SetNextWindowClass(&windowClass);
        createToolBar();
        // Set the window class for the next window
        ImGui::SetNextWindowClass(&windowClass);
        inspectorPanel.update(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

        // Set the window class for the next window
        ImGui::SetNextWindowClass(&windowClass);
        sceneHierarchyPanel.update(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

        ImGui::SetNextWindowClass(&windowClass);
        sceneView.update(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

        ImGui::SetNextWindowClass(&windowClass);
        gameView.update(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    }

    void ClientPanel::createToolBar() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto& colors = ImGui::GetStyle().Colors;
        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##ClientToolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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
            } else {
                stopRunningScene();
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        ImGui::End();
    }

    void ClientPanel::startRunningScene() {
#ifdef TE_DEBUG
        compileClientUserScripts(DEBUG);
#else
        compileClientUserScripts(RELEASEDEBUG);
#endif
        client.sceneManager.saveSceneAsTemporarily(client.sceneManager.getActiveSceneName());
        EventDispatcher::getInstance().copy();
        client.materialManager.copy();
        ScriptEngine::getInstance()->init(projectManager.getClientUserScriptsDLLPath().string());
        ScriptEngine::getInstance()->onStart();
        client.physicsEngine.start();
        m_currentPlaying = true;
    }

    void ClientPanel::stopRunningScene() {
        client.physicsEngine.stop();
        EventDispatcher::getInstance().restoreCopy();
        client.materialManager.restoreCopy();
        client.sceneManager.loadSceneFromTemporarily(client.sceneManager.getActiveSceneName());
        sceneHierarchyPanel.getSelectedGO().clear();
        for (GameObject* gameObject: client.sceneManager.getScene().getGameObjects()) {
            if (std::find(sceneHierarchyPanel.getSelectedGO().begin(), sceneHierarchyPanel.getSelectedGO().end(), gameObject) != sceneHierarchyPanel.getSelectedGO().end()) {
                sceneHierarchyPanel.selectGO(gameObject);
            }
        }
        m_currentPlaying = false;
        ScriptEngine::getInstance()->stop();
    }

    void ClientPanel::compileClientUserScripts(CompileMode compileMode) {
        if (!exists(projectManager.getClientCmakeBuildPath()) || is_empty(projectManager.getClientCmakeBuildPath())) {
            std::string command = "\"" + projectManager.getCmakePath().string() +
                                  " -G \"Visual Studio 17 2022\""
                                  " -D TechEngineClientLIB:STRING=\"" + projectManager.getTechEngineClientLibPath().string() + "\"" +
                                  " -D TechEngineCoreLIB:STRING=\"" + projectManager.getTechEngineCoreLibPath().string() + "\"" +
                                  " -S " + "\"" + projectManager.getClientCmakeListPath().string() + "\"" +
                                  " -B " + "\"" + projectManager.getClientCmakeBuildPath().string() + "\"" + "\"";
            std::system(command.c_str());
        }
        std::string cm;
        if (compileMode == CompileMode::RELEASE) {
            cm = "Release";
        } else if (compileMode == CompileMode::RELEASEDEBUG) {
            cm = "RelWithDebInfo";
        } else if (compileMode == CompileMode::DEBUG) {
            cm = "Debug";
        }
        std::string command = "\"" + projectManager.getCmakePath().string() +
                              " --build " + "\"" + projectManager.getClientCmakeBuildPath().string() + "\"" + " --target UserScripts --config " + cm + "\"";
        std::system(command.c_str());
        TE_LOGGER_INFO("Build finished! {0}", cm);
    }
}
