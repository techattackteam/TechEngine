#include "ImGuiUtils.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "core/Logger.hpp"

namespace TechEngine {

    bool TechEngine::ImGuiUtils::beginMenuWithInputMenuField(const std::string &menuName, const std::string &hint, std::string &output) {
        bool result = false;
        if (ImGui::BeginMenu((const char *) menuName.c_str())) {
            //create text input for scene name
            const char *hintBuffer = hint.c_str();
            ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
            flags |= ImGuiInputTextFlags_AutoSelectAll;
            if (ImGui::InputTextWithHint("##", hintBuffer, &output, flags)) {
                ImGui::CloseCurrentPopup();
                result = true;
            } else {
                result = false;
            }
            ImGui::EndMenu();
        }
        return result;
    }
}
