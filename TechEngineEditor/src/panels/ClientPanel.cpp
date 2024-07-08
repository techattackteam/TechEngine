#include "ClientPanel.hpp"

#include "PanelsManager.hpp"
#include "events/scripts/ScriptCrashEvent.hpp"
#include "script/ScriptEngine.hpp"
#include "network/NetworkEngine.hpp"

namespace TechEngine {
    ClientPanel::ClientPanel(Client& client,
                             SystemsRegistry& editorRegistry,
                             SystemsRegistry& clientRegistry,
                             PanelsManager& panelsManager) : client(client),
                                                             editorRegistry(editorRegistry),
                                                             appRegistry(clientRegistry),
                                                             panelsManager(panelsManager),
                                                             gameView(editorRegistry, clientRegistry),
                                                             inspectorPanel("Client Inspector", editorRegistry, clientRegistry, sceneHierarchyPanel.getSelectedGO()),
                                                             sceneView("Client Scene", editorRegistry, clientRegistry, sceneHierarchyPanel.getSelectedGO()),
                                                             sceneHierarchyPanel("Client Scene Hierarchy", editorRegistry, clientRegistry),
                                                             Panel("ClientPanel", editorRegistry) {
    }

    void ClientPanel::init() {
        Panel::init();
        inspectorPanel.init();
        sceneHierarchyPanel.init();
        sceneView.init();
        gameView.init();

        client.systemsRegistry.getSystem<EventDispatcher>().subscribe(ScriptCrashEvent::eventType, [this](Event* event) {
            stopRunningScene();
        });
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
        inspectorPanel.update();

        // Set the window class for the next window
        ImGui::SetNextWindowClass(&windowClass);
        sceneHierarchyPanel.update();

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
        panelsManager.compileUserScripts(DEBUG, CompileProject::PROJECT_CLIENT);
#else
        panelsManager.compileUserScripts(RELEASEDEBUG, CompileProject::PROJECT_CLIENT);
#endif
        appRegistry.getSystem<SceneManager>().saveSceneAsTemporarily(editorRegistry.getSystem<ProjectManager>().getProjectCachePath().string(), CompileProject::PROJECT_CLIENT);
        appRegistry.getSystem<EventDispatcher>().copy();
        appRegistry.getSystem<MaterialManager>().copy();
        appRegistry.getSystem<ScriptEngine>().init(editorRegistry.getSystem<ProjectManager>().getClientUserScriptsDLLPath().string(), &appRegistry.getSystem<EventDispatcher>());
        appRegistry.getSystem<ScriptEngine>().onStart();
        appRegistry.getSystem<PhysicsEngine>().start();
        m_currentPlaying = true;
    }

    void ClientPanel::stopRunningScene() {
        appRegistry.getSystem<PhysicsEngine>().stop();
        appRegistry.getSystem<EventDispatcher>().restoreCopy();
        appRegistry.getSystem<MaterialManager>().restoreCopy();
        appRegistry.getSystem<SceneManager>().loadSceneFromTemporarily(editorRegistry.getSystem<ProjectManager>().getProjectCachePath().string(), CompileProject::PROJECT_CLIENT);
        sceneHierarchyPanel.getSelectedGO().clear();
        for (GameObject* gameObject: appRegistry.getSystem<SceneManager>().getScene().getGameObjects()) {
            if (std::find(sceneHierarchyPanel.getSelectedGO().begin(), sceneHierarchyPanel.getSelectedGO().end(), gameObject) != sceneHierarchyPanel.getSelectedGO().end()) {
                sceneHierarchyPanel.selectGO(gameObject);
            }
        }
        m_currentPlaying = false;
        appRegistry.getSystem<ScriptEngine>().stop();
        if (appRegistry.getSystem<NetworkEngine>().isRunning()) {
            appRegistry.getSystem<NetworkEngine>().disconnectServer();
        }
    }
}
