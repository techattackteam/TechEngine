#include <ui/ClosePrompt.hpp>

#include <imgui.h>

namespace TechEngine {
    static constexpr const char* CLOSE_PROMPT_NAME = "Unsaved changes";

    void ClosePrompt::open() {
        m_openRequested = true;
    }

    CloseChoice ClosePrompt::draw() {
        // OpenPopup and BeginPopupModal resolve the name against the same ID stack, so both stay here.
        if (m_openRequested) {
            ImGui::OpenPopup(CLOSE_PROMPT_NAME);
            m_openRequested = false;
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (!ImGui::BeginPopupModal(CLOSE_PROMPT_NAME, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            return CloseChoice::None;
        }

        ImGui::TextUnformatted("The scene has unsaved changes. Save them before closing?");
        ImGui::Spacing();

        CloseChoice choice = CloseChoice::None;
        if (ImGui::Button("Save")) {
            choice = CloseChoice::Save;
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard")) {
            choice = CloseChoice::Discard;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            choice = CloseChoice::Cancel;
        }

        if (choice != CloseChoice::None) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return choice;
    }
}
