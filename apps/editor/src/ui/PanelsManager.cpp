#include <ui/PanelsManager.hpp>

#include <imgui.h>
#include <imgui_internal.h>

namespace TechEngine {
    static constexpr const char* DOCK_SPACE_NAME = "EditorDockSpace";
    static constexpr float HIERARCHY_WIDTH_RATIO = 0.2f;
    // Split from what the hierarchy leaves, so 0.25 of the remaining 80% is a fifth of the window.
    static constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;

    PanelsManager::PanelsManager(EditorCommands& commands) : m_hierarchy(commands), m_inspector(commands) {
    }

    void PanelsManager::draw(const EditorSnapshot& snapshot) {
        // A dockspace hosts only the windows submitted after it in the same frame.
        drawDockSpace();
        m_hierarchy.draw(snapshot);
        m_inspector.draw(snapshot);
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
        const ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, INSPECTOR_WIDTH_RATIO, nullptr, &centre);
        ImGui::DockBuilderDockWindow(m_hierarchy.getName().c_str(), left);
        ImGui::DockBuilderDockWindow(m_inspector.getName().c_str(), right);
        ImGui::DockBuilderFinish(dockSpaceId);
    }
}
