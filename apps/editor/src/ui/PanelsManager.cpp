#include <edit/EditorCommands.hpp>
#include <ui/PanelsManager.hpp>

#include <imgui.h>
#include <imgui_internal.h>

namespace TechEngine {
    static constexpr const char* DOCK_SPACE_NAME = "EditorDockSpace";
    static constexpr float HIERARCHY_WIDTH_RATIO = 0.2f;
    // Split from what the hierarchy leaves, so 0.25 of the remaining 80% is a fifth of the window.
    static constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;

    PanelsManager::PanelsManager(EditorCommands& commands) : m_commands(&commands), m_hierarchy(commands), m_inspector(commands) {
    }

    void PanelsManager::draw(const EditorSnapshot& snapshot) {
        // The menu bar takes its height from the viewport's work area, which the dockspace then fills.
        drawMainMenuBar();
        handleShortcuts();
        // A dockspace hosts only the windows submitted after it in the same frame.
        drawDockSpace();
        m_hierarchy.draw(snapshot);
        m_inspector.draw(snapshot);
        m_sceneView.draw(snapshot);
        m_gameView.draw(snapshot);
    }

    void PanelsManager::drawMainMenuBar() {
        if (!ImGui::BeginMainMenuBar()) {
            return;
        }
        if (ImGui::BeginMenu("File")) {
            // The shortcut text is only drawn; handleShortcuts() is what reacts to Ctrl+S.
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                m_commands->save();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    void PanelsManager::handleShortcuts() {
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) {
            m_commands->save();
        }
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
        ImGui::DockBuilderDockWindow(m_sceneView.getName().c_str(), centre);
        ImGui::DockBuilderDockWindow(m_gameView.getName().c_str(), centre);
        ImGui::DockBuilderFinish(dockSpaceId);
    }
}
