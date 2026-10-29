#include <ui/PanelsManager.hpp>

#include <imgui.h>
#include <imgui_internal.h>

namespace TechEngine {
    static constexpr const char* DOCK_SPACE_NAME = "EditorDockSpace";
    static constexpr float HIERARCHY_WIDTH_RATIO = 0.2f;

    PanelsManager::PanelsManager(EditorCommands& commands) : m_hierarchy(commands) {
    }

    void PanelsManager::draw(const EditorSnapshot& snapshot) {
        // A dockspace hosts only the windows submitted after it in the same frame.
        drawDockSpace();
        m_hierarchy.draw(snapshot);
    }

    void PanelsManager::drawDockSpace() {
        const ImGuiID dockSpaceId = ImGui::GetID(DOCK_SPACE_NAME);
        const ImGuiDockNode* existing = ImGui::DockBuilderGetNode(dockSpaceId);
        if (existing == nullptr) {
            buildDefaultLayout(dockSpaceId);
        }
        ImGui::DockSpaceOverViewport(dockSpaceId, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    }

    void PanelsManager::buildDefaultLayout(const ImGuiID dockSpaceId) {
        ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockSpaceId, ImGui::GetMainViewport()->WorkSize);

        ImGuiID centre = dockSpaceId;
        const ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, HIERARCHY_WIDTH_RATIO, nullptr, &centre);
        ImGui::DockBuilderDockWindow(m_hierarchy.getName().c_str(), left);
        ImGui::DockBuilderFinish(dockSpaceId);
    }
}
