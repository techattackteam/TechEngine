#include <ui/panels/SceneViewPanel.hpp>

#include <imgui.h>

namespace TechEngine {
    SceneViewPanel::SceneViewPanel() : Panel("Scene") {
    }

    void SceneViewPanel::onDraw(const EditorSnapshot&) {
        const char* hint = "The scene view draws nothing until the renderer exists.";
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const ImVec2 hintSize = ImGui::CalcTextSize(hint);
        const ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursor.x + std::max(0.0f, (available.x - hintSize.x) * 0.5f), cursor.y + std::max(0.0f, (available.y - hintSize.y) * 0.5f)));
        ImGui::TextDisabled("%s", hint);
    }
}
