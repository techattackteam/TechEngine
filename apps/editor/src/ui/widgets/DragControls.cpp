#include <ui/widgets/DragControls.hpp>

#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

namespace TechEngine {
    struct HeldDrag {
        ImGuiID id = 0;
        std::array<std::byte, 8> value{};
    };

    static HeldDrag g_heldDrag;

    static constexpr float VEC3_DRAG_SPEED = 0.1f;
    static constexpr const char* VEC3_FORMAT = "%.2f";
    static constexpr float AXIS_GAP = 4.0f;
    static constexpr float AXIS_HOVER_LIFT = 0.1f;
    static constexpr std::array<const char*, 3> AXIS_LETTERS = {"X", "Y", "Z"};
    static constexpr std::array<ImVec4, 3> AXIS_COLORS = {
        ImVec4(0.80f, 0.15f, 0.15f, 1.0f),
        ImVec4(0.20f, 0.65f, 0.20f, 1.0f),
        ImVec4(0.15f, 0.35f, 0.85f, 1.0f),
    };

    bool dragScalar(const char* label, const ImGuiDataType type, void* value, const float speed, const char* format) {
        const std::size_t size = ImGui::DataTypeGetInfo(type)->Size;
        const ImGuiID id = ImGui::GetID(label);
        if (g_heldDrag.id == id) {
            std::memcpy(value, g_heldDrag.value.data(), size);
        }

        const bool changed = ImGui::DragScalar(label, type, value, speed, nullptr, nullptr, format);
        if (ImGui::IsItemActive()) {
            g_heldDrag.id = id;
            std::memcpy(g_heldDrag.value.data(), value, size);
        } else if (g_heldDrag.id == id) {
            g_heldDrag.id = 0;
        }
        return changed;
    }

    bool drawVec3Control(const char* id, Vec3& values, const float resetValue, std::array<bool, 3>& changedAxes) {
        ImGui::PushID(id);
        const float lineHeight = ImGui::GetFrameHeight();
        const ImVec2 letterSize(lineHeight, lineHeight);
        const float dragWidth = std::max(1.0f, (ImGui::GetContentRegionAvail().x - 2.0f * AXIS_GAP) / 3.0f - letterSize.x);

        bool changed = false;
        for (std::size_t axis = 0; axis < 3; axis++) {
            const int component = static_cast<int>(axis);
            ImGui::PushID(component);
            if (axis > 0) {
                ImGui::SameLine(0.0f, AXIS_GAP);
            }
            changedAxes[axis] = false;

            const ImVec4& color = AXIS_COLORS[axis];
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x + AXIS_HOVER_LIFT, color.y + AXIS_HOVER_LIFT, color.z + AXIS_HOVER_LIFT, color.w));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
            if (ImGui::Button(AXIS_LETTERS[axis], letterSize)) {
                values[component] = resetValue;
                changedAxes[axis] = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine(0.0f, 0.0f);
            ImGui::SetNextItemWidth(dragWidth);
            const bool dragged = dragScalar("##drag", ImGuiDataType_Float, &values[component], VEC3_DRAG_SPEED, VEC3_FORMAT);
            changedAxes[axis] = changedAxes[axis] || dragged;
            changed = changed || changedAxes[axis];
            ImGui::PopID();
        }

        ImGui::PopID();
        return changed;
    }
}
