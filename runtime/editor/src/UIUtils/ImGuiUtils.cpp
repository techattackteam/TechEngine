#include "core/Logger.hpp"
#include "ImGuiUtils.hpp"
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"

namespace TechEngine {
    bool ImGuiUtils::beginMenuWithInputMenuField(const std::string& menuName, const std::string& hint, std::string& output) {
        bool result = false;
        if (ImGui::BeginMenu((const char*)menuName.c_str())) {
            //create text input for scene name
            const char* hintBuffer = hint.c_str();
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

    void ImGuiUtils::drawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth, float min, float max, bool enable) {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();

        ImGui::SameLine();
        if (!enable) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.1f);
            ImGui::DragFloat("##X", &values.x, 0.1f, min, max, "%.2f");
            ImGui::PopStyleVar();
        } else {
            ImGui::DragFloat("##X", &values.x, 0.1f, min, max, "%.2f");
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();

        ImGui::SameLine();
        if (!enable) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.1f);
            ImGui::DragFloat("##Y", &values.y, 0.1f, min, max, "%.2f");
            ImGui::PopStyleVar();
        } else {
            ImGui::DragFloat("##Y", &values.y, 0.1f, min, max, "%.2f");
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopFont();

        ImGui::SameLine();
        if (!enable) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.1f);
            ImGui::DragFloat("##Z", &values.z, 0.1f, min, max, "%.2f");
            ImGui::PopStyleVar();
        } else {
            ImGui::DragFloat("##Z", &values.z, 0.1f, min, max, "%.2f");
        }
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }
}
