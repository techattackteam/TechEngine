#include "ClientPanel.hpp"

namespace TechEngine {
    ClientPanel::ClientPanel(Client& client) : client(client),
                                               gameView(client.renderer, client.sceneManager.getScene()),
                                               inspectorPanel("Client Inspector", sceneHierarchyPanel.getSelectedGO(), client.materialManager, client.physicsEngine),
                                               sceneView("Client Scene", client.renderer, client.sceneManager.getScene(), client.physicsEngine, sceneHierarchyPanel.getSelectedGO()),
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
                //startRunningScene();
            } else {
                //stopRunningScene();
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        ImGui::End();
    }
}
