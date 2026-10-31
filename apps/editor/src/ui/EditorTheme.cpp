#include <ui/EditorTheme.hpp>

#include <imgui.h>

namespace TechEngine {
    static ImVec4 accent(const float alpha) {
        return ImVec4(0.26f, 0.59f, 0.98f, alpha);
    }

    static ImVec4 gray(const float value, const float alpha = 1.0f) {
        return ImVec4(value, value, value, alpha);
    }

    void applyEditorTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text] = gray(0.80f);
        colors[ImGuiCol_TextDisabled] = gray(0.50f);
        colors[ImGuiCol_WindowBg] = gray(0.15f);
        colors[ImGuiCol_Border] = gray(0.30f, 0.30f);
        colors[ImGuiCol_BorderShadow] = gray(0.10f, 0.20f);
        colors[ImGuiCol_FrameBg] = gray(0.20f);
        colors[ImGuiCol_FrameBgHovered] = accent(0.40f);
        colors[ImGuiCol_FrameBgActive] = accent(0.67f);
        colors[ImGuiCol_TitleBg] = gray(0.13f);
        colors[ImGuiCol_TitleBgActive] = gray(0.13f);
        colors[ImGuiCol_TitleBgCollapsed] = gray(0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = gray(0.20f);
        colors[ImGuiCol_ScrollbarBg] = gray(0.10f);
        colors[ImGuiCol_ScrollbarGrab] = gray(0.31f);
        colors[ImGuiCol_ScrollbarGrabHovered] = accent(0.67f);
        colors[ImGuiCol_ScrollbarGrabActive] = accent(1.00f);
        colors[ImGuiCol_CheckMark] = accent(1.00f);
        colors[ImGuiCol_SliderGrab] = gray(0.45f);
        colors[ImGuiCol_SliderGrabActive] = accent(1.00f);
        colors[ImGuiCol_Button] = gray(0.40f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = accent(0.67f);
        colors[ImGuiCol_ButtonActive] = accent(1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.19f, 0.29f, 0.41f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = gray(0.25f);
        colors[ImGuiCol_HeaderActive] = accent(1.00f);
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered] = accent(0.78f);
        colors[ImGuiCol_SeparatorActive] = accent(1.00f);
        colors[ImGuiCol_ResizeGrip] = gray(0.40f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = accent(0.67f);
        colors[ImGuiCol_ResizeGripActive] = accent(1.00f);
        colors[ImGuiCol_Tab] = gray(0.18f);
        colors[ImGuiCol_TabHovered] = gray(0.30f);
        colors[ImGuiCol_TabSelected] = gray(0.24f);
        colors[ImGuiCol_TabSelectedOverline] = accent(1.00f);
        colors[ImGuiCol_TabDimmed] = colors[ImGuiCol_Tab];
        colors[ImGuiCol_TabDimmedSelected] = colors[ImGuiCol_TabSelected];
        colors[ImGuiCol_TabDimmedSelectedOverline] = gray(0.00f, 0.00f);
        colors[ImGuiCol_DockingPreview] = accent(0.70f);
        colors[ImGuiCol_DockingEmptyBg] = gray(0.20f);
        colors[ImGuiCol_DragDropTarget] = accent(1.00f);

        style.WindowRounding = 2.0f;
        style.ChildRounding = 2.0f;
        style.PopupRounding = 2.0f;
        style.FrameRounding = 2.0f;
        style.FrameBorderSize = 1.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 2.0f;
        style.TabBarOverlineSize = 2.0f;
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    }
}
