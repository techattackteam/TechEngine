#include <edit/EditorCommands.hpp>
#include <ui/PanelsManager.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include <string>
#include <utility>

namespace TechEngine {
    static constexpr const char* DOCK_SPACE_NAME = "EditorDockSpace";
    static constexpr float HIERARCHY_WIDTH_RATIO = 0.2f;
    // Split from what the hierarchy leaves, so 0.25 of the remaining 80% is a fifth of the window.
    static constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;
    static constexpr float CONSOLE_HEIGHT_RATIO = 0.3f;
    static constexpr const char* PLACE_MODEL_POPUP_NAME = "Place model";
    static constexpr const char* PLACE_MODEL_PATH_HINT = "project://assets/client/Sponza/Sponza.temodel";
    static constexpr float PLACE_MODEL_INPUT_WIDTH_EMS = 32.0f;

    PanelsManager::PanelsManager(EditorCommands& commands, ConsoleHistory& consoleHistory, std::function<void()> exportProject, std::function<void()> runProject)
        : m_commands(&commands), m_exportProject(std::move(exportProject)), m_runProject(std::move(runProject)), m_hierarchy(commands), m_inspector(commands), m_console(consoleHistory) {
    }

    void PanelsManager::draw(const EditorSnapshot& snapshot, const bool canRun) {
        // The menu bar takes its height from the viewport's work area, which the dockspace then fills.
        drawMainMenuBar(canRun);
        drawPlaceModelPopup();
        handleShortcuts();
        // A dockspace hosts only the windows submitted after it in the same frame.
        drawDockSpace();
        m_hierarchy.draw(snapshot);
        m_inspector.draw(snapshot);
        m_sceneView.draw(snapshot);
        m_gameView.draw(snapshot);
        m_console.draw(snapshot);
    }

    void PanelsManager::drawMainMenuBar(const bool canRun) {
        if (!ImGui::BeginMainMenuBar()) {
            return;
        }
        if (ImGui::BeginMenu("File")) {
            // The shortcut text is only drawn; handleShortcuts() is what reacts to Ctrl+S.
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                m_commands->save();
            }
            if (ImGui::MenuItem("Export")) {
                m_exportProject();
            }
            if (ImGui::MenuItem("Run", nullptr, false, canRun)) {
                m_runProject();
            }
            // TODO(E2): the asset browser places a model and replaces this entry.
            if (ImGui::MenuItem("Place model...")) {
                m_placeModelRequested = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    void PanelsManager::drawPlaceModelPopup() {
        // OpenPopup and BeginPopupModal resolve the name against the same ID stack, so both stay here.
        if (m_placeModelRequested) {
            ImGui::OpenPopup(PLACE_MODEL_POPUP_NAME);
            m_placeModelRequested = false;
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (!ImGui::BeginPopupModal(PLACE_MODEL_POPUP_NAME, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            return;
        }

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(ImGui::GetFontSize() * PLACE_MODEL_INPUT_WIDTH_EMS);
        const bool entered = ImGui::InputTextWithHint("##path", PLACE_MODEL_PATH_HINT, m_placeModelPath.data(), m_placeModelPath.size(), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::Spacing();

        const bool hasPath = m_placeModelPath[0] != '\0';
        ImGui::BeginDisabled(!hasPath);
        const bool placeClicked = ImGui::Button("Place");
        ImGui::EndDisabled();
        ImGui::SameLine();
        const bool cancelled = ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false);

        if (hasPath && (entered || placeClicked)) {
            m_commands->placeModel(std::string(m_placeModelPath.data()));
            ImGui::CloseCurrentPopup();
        } else if (cancelled) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
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
        const ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, CONSOLE_HEIGHT_RATIO, nullptr, &centre);
        ImGui::DockBuilderDockWindow(m_hierarchy.getName().c_str(), left);
        ImGui::DockBuilderDockWindow(m_inspector.getName().c_str(), right);
        ImGui::DockBuilderDockWindow(m_console.getName().c_str(), bottom);
        ImGui::DockBuilderDockWindow(m_sceneView.getName().c_str(), centre);
        ImGui::DockBuilderDockWindow(m_gameView.getName().c_str(), centre);
        ImGui::DockBuilderFinish(dockSpaceId);
    }
}
