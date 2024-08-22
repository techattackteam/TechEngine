#include "ServerPanel.hpp"

#include <imgui_internal.h>

namespace TechEngine {
    void ServerPanel::onInit() {
        m_TestPanel.init("Server Test Panel", &m_dockSpaceWindowClass);
    }

    void ServerPanel::onUpdate() {
        m_TestPanel.update();
    }

    void ServerPanel::setupInitialDockingLayout() {
        ImGui::DockBuilderRemoveNode(m_dockSpaceID); // Clear any previous layout
        ImGui::DockBuilderAddNode(m_dockSpaceID, ImGuiDockNodeFlags_DockSpace); // Create a new dockspace node

        ImGuiID test_A = ImGui::DockBuilderAddNode(m_dockSpaceID, ImGuiDockNodeFlags_DockSpace);

        ImGui::DockBuilderDockWindow(m_TestPanel.getName().c_str(), test_A);

        ImGui::DockBuilderFinish(m_dockSpaceID); // Finalize the layout
    }

    void ServerPanel::createToolBar() {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto& colors = ImGui::GetStyle().Colors;
        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));


        float size = ImGui::GetWindowWidth() / 10;
        ImVec2 pos = ImGui::GetContentRegionAvail();
        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPosX(pos.x / 4 - size / 2);
        ImGui::SetCursorPosY(cursor.y + 4);
        if (ImGui::Button("World", ImVec2(size, 0))) {
            //sceneView.changeGuizmoMode(sceneView.getGuizmoMode() == ImGuizmo::MODE::WORLD ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(pos.x / 2 - (size / 2));
        if (ImGui::Button("Play", ImVec2(size, 0))) {
            /*if (!m_currentPlaying) {
                startRunningScene();
            } else {
                stopRunningScene();
            }*/
        }
        ImGui::PopStyleColor(3);
    }
}
