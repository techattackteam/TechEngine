#include "ServerPanel.hpp"

#include "core/Client.hpp"
#include "PanelsManager.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    ServerPanel::ServerPanel(PanelsManager& panelsManager,
                             EventDispatcher& eventDispatcher,
                             Server& server,
                             ProjectManager& projectManager,
                             Renderer& renderer) : server(server),
                                                   panelsManager(panelsManager),
                                                   projectManager(projectManager),
                                                   inspectorPanel("Server Inspector", eventDispatcher, sceneHierarchyPanel.getSelectedGO(), server.materialManager, server.physicsEngine),
                                                   sceneView("Server Scene", renderer, server.sceneManager.getScene(), server.physicsEngine, eventDispatcher, sceneHierarchyPanel.getSelectedGO()),
                                                   sceneHierarchyPanel("Server Scene Hierarchy", eventDispatcher, server.sceneManager.getScene(), server.materialManager),
                                                   Panel("ServerPanel", eventDispatcher) {
    }

    void ServerPanel::onUpdate() {
        // Create a dockspace in the window
        ImGuiID dockspaceId = ImGui::GetID("ServerDockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // Create an ImGuiWindowClass and set the DockNodeFlagsOverrideSet field
        ImGuiWindowClass windowClass;
        windowClass.ClassId = dockspaceId;
        windowClass.DockingAllowUnclassed = false;
        ImGui::SetNextWindowClass(&windowClass);
        createToolBar();
        // Set the window class for the next window
        ImGui::SetNextWindowClass(&windowClass);
        inspectorPanel.update();

        // Set the window class for the next window
        ImGui::SetNextWindowClass(&windowClass);
        sceneHierarchyPanel.update();

        ImGui::SetNextWindowClass(&windowClass);
        sceneView.update(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    }

    void ServerPanel::createToolBar() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto& colors = ImGui::GetStyle().Colors;
        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##ServerToolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
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

    bool ServerPanel::isRunning() const {
        return m_currentPlaying;
    }

    void ServerPanel::startRunningScene() {
#ifdef TE_DEBUG
        panelsManager.compileUserScripts(DEBUG, PROJECT_SERVER);
#else
        panelsManager.compileUserScripts(RELEASEDEBUG, PROJECT_SERVER);
#endif
        server.sceneManager.saveSceneAsTemporarily(projectManager.getProjectCachePath().string(), PROJECT_SERVER);
        server.eventDispatcher.copy();
        server.materialManager.copy();
        server.scriptEngine.init(projectManager.getServerUserScriptsDLLPath().string());
        server.scriptEngine.onStart();
        server.physicsEngine.start();
        m_currentPlaying = true;
    }

    void ServerPanel::stopRunningScene() {
        server.physicsEngine.stop();
        server.eventDispatcher.restoreCopy(); //PROBLEM: EventDispatcher is a singleton while there is a client and server instance
        server.materialManager.restoreCopy();
        server.sceneManager.loadSceneFromTemporarily(projectManager.getProjectCachePath().string(), PROJECT_SERVER);
        sceneHierarchyPanel.getSelectedGO().clear();
        for (GameObject* gameObject: server.sceneManager.getScene().getGameObjects()) {
            if (std::find(sceneHierarchyPanel.getSelectedGO().begin(), sceneHierarchyPanel.getSelectedGO().end(), gameObject) != sceneHierarchyPanel.getSelectedGO().end()) {
                sceneHierarchyPanel.selectGO(gameObject);
            }
        }
        m_currentPlaying = false;
        server.scriptEngine.stop(); //PROBLEM: ScriptEngine is a singleton while there is a client and server instance
    }
}
