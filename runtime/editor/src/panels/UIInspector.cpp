#include "UIInspector.hpp"

#include "ui/Widget.hpp"
#include "imgui_stdlib.h"
#include "core/Logger.hpp"
#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/InputTextWidget.hpp"
#include "ui/InteractableWidget.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/TextWidget.hpp"
#include "ui/WidgetsRegistry.hpp"

namespace TechEngine {
    UIInspector::UIInspector(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry) : m_appSystemsRegistry(appSystemsRegistry), Panel(editorSystemsRegistry) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        m_styleVars.emplace_back(ImGuiStyleVar_WindowBorderSize, 0.0f);
        m_styleVars.emplace_back(ImGuiStyleVar_WindowRounding, 0.0f);
    }

    void UIInspector::onInit() {
    }

    void UIInspector::onUpdate() {
        if (m_selectedWidget) {
            bool changed = false;

            if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushID("Position");

                // --- ANCHOR PRESET DROPDOWN ---
                const char* anchorPresetNames[] = {
                    "Top Left", "Top Center", "Top Right",
                    "Middle Left", "Middle Center", "Middle Right",
                    "Bottom Left", "Bottom Center", "Bottom Right",
                    "Stretch Top", "Stretch Middle", "Stretch Bottom",
                    "Stretch Left", "Stretch Center", "Stretch Right",
                    "Stretch Fill"
                };
                int currentPreset = static_cast<int>(m_selectedWidget->m_preset);
                if (ImGui::Combo("Anchor Preset", &currentPreset, anchorPresetNames, IM_ARRAYSIZE(anchorPresetNames))) {
                    UIRenderer& uiRenderer = m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer();
                    glm::vec4 rootScreenRect = {0.0f, 0.0f, (float)uiRenderer.m_screenWidth, (float)uiRenderer.m_screenHeight};
                    glm::vec4 parentScreenRect = m_selectedWidget->m_parent ? m_selectedWidget->m_parent->m_finalScreenRect : rootScreenRect;
                    m_selectedWidget->changeAnchor(static_cast<Widget::AnchorPreset>(currentPreset), parentScreenRect, uiRenderer.getDpiScale());

                    changed = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Determines how this UI element attaches to its parent.\nThis changes which position and size fields are available below.");
                }

                // --- PIVOT SLIDER ---
                if (ImGui::SliderFloat2("Pivot", &m_selectedWidget->m_pivot.x, 0.0f, 1.0f, "%.2f")) {
                    changed = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("The element's own origin point for positioning, scaling, and rotation.\n(0, 0) is bottom-left, (0.5, 0.5) is center, (1, 1) is top-right.");
                }

                ImGui::Separator();

                // --- DYNAMIC POSITION & SIZE FIELDS ---
                bool isStretchingX = m_selectedWidget->m_anchorMax.x - m_selectedWidget->m_anchorMin.x > 0.001f;
                bool isStretchingY = m_selectedWidget->m_anchorMax.y - m_selectedWidget->m_anchorMin.y > 0.001f;

                // --- X-AXIS CONTROLS ---
                ImGui::Text("Horizontal Axis");
                if (isStretchingX) {
                    // If stretching horizontally, control Left and Right margins
                    changed |= ImGui::DragFloat("Left", &m_selectedWidget->m_left, 1.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance from the left anchor to the left edge.");

                    changed |= ImGui::DragFloat("Right", &m_selectedWidget->m_right, 1.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance from the right anchor to the right edge.");
                } else {
                    // If not stretching, control Position X and a fixed Width
                    changed |= ImGui::DragFloat("Pos X", &m_selectedWidget->m_anchoredPosition.x, 1.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Position of the pivot on the X-axis, relative to the anchor point.");

                    changed |= ImGui::DragFloat("Width", &m_selectedWidget->m_size.x, 1.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The fixed width of the element.");
                }

                // --- Y-AXIS CONTROLS ---
                ImGui::Text("Vertical Axis");
                if (isStretchingY) {
                    // If stretching vertically, control Top and Bottom margins
                    changed |= ImGui::DragFloat("Top", &m_selectedWidget->m_top, 1.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance from the top anchor to the top edge.");

                    changed |= ImGui::DragFloat("Bottom", &m_selectedWidget->m_bottom, 1.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance from the bottom anchor to the bottom edge.");
                } else {
                    // If not stretching, control Position Y and a fixed Height
                    changed |= ImGui::DragFloat("Pos Y", &m_selectedWidget->m_anchoredPosition.y, 1.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Position of the pivot on the Y-axis, relative to the anchor point.");

                    changed |= ImGui::DragFloat("Height", &m_selectedWidget->m_size.y, 1.0f);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The fixed height of the element.");
                }

                // --- ROTATION Z ---
                if (ImGui::SliderFloat("Rotation Z", &m_selectedWidget->m_rotationZ, -180.0f, 180.0f, "%.1f°")) {
                    changed = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Rotation of the element in degrees around the pivot point.\nPositive values rotate clockwise.");
                }

                ImGui::PopID();
            }

            if (changed) {
                m_appSystemsRegistry.getSystem<WidgetsRegistry>().calculateWidgetLayout(m_selectedWidget);
            }
        }


        if (!m_selectedWidget) {
            return;
        }

        ImGui::Separator();
        if (dynamic_cast<PanelWidget*>(m_selectedWidget.get())) {
            ImGui::Text("Panel:");
            PanelWidget* widget = dynamic_cast<PanelWidget*>(m_selectedWidget.get());
            const glm::vec4& currentColor = widget->getBackgroundColor();

            float colorsFloat[4] = {
                currentColor.x,
                currentColor.y,
                currentColor.z,
                currentColor.a
            };
            if (ImGui::ColorPicker4("##ColorValue", colorsFloat)) {
                TE_LOGGER_INFO("Setting color property '({0} , {1}, {2}, {3})'", colorsFloat[0], colorsFloat[1], colorsFloat[2], colorsFloat[3]);
                widget->setBackgroundColor({colorsFloat[0], colorsFloat[1], colorsFloat[2], colorsFloat[3]});
            }
        } else if (dynamic_cast<TextWidget*>(m_selectedWidget.get())) {
            ImGui::Text("Text Content:");
            ImGui::PushID("Text");
            inspectTextWidget(dynamic_cast<TextWidget*>(m_selectedWidget.get()));
            ImGui::PopID();
        } else if (dynamic_cast<InputTextWidget*>(m_selectedWidget.get())) {
            ImGui::Text("Input Text:");
            ImGui::PushID("InputText");
            inspectTextWidget(dynamic_cast<InputTextWidget*>(m_selectedWidget.get()));
            ImGui::PopID();
        } else if (dynamic_cast<InteractableWidget*>(m_selectedWidget.get())) {
            ImGui::Text("Interactable Widget:");
            InteractableWidget* widget = dynamic_cast<InteractableWidget*>(m_selectedWidget.get());
            bool clickable = widget->isClickable();
            if (ImGui::Checkbox("Clickable", &clickable)) {
                widget->setClickable(clickable);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If unchecked, this widget will not respond to user interactions.");
            }
        }
    }

    void UIInspector::setSelectedWidget(std::shared_ptr<Widget> widget) {
        this->m_selectedWidget = widget;
    }

    bool UIInspector::resizableInputTextMultiline(const char* label, std::string* text, const ImVec2& initial_size = ImVec2(0, 0), float min_height = 30.0f) {
        ImGui::PushID(label);

        static std::map<ImGuiID, float> heights;
        const ImGuiID id = ImGui::GetID("##texteditor");

        if (heights.find(id) == heights.end()) {
            heights[id] = initial_size.y > 0 ? initial_size.y : ImGui::GetTextLineHeight() * 4;
        }
        float& height = heights[id];

        bool changed = ImGui::InputTextMultiline("##editor", text, ImVec2(-FLT_MIN, height));

        const float handle_height = 8.0f; // The thickness of our drag handle

        ImGui::InvisibleButton("##resizer", ImVec2(-FLT_MIN, handle_height));

        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }

        if (ImGui::IsItemActive()) {
            float drag_delta_y = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f).y;

            height += drag_delta_y;

            if (height < min_height) {
                height = min_height;
            }

            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 handle_min = ImGui::GetItemRectMin();
        const ImVec2 handle_max = ImGui::GetItemRectMax();
        const float handle_line_y = handle_min.y + handle_height / 2.0f;
        const float handle_width = ImGui::GetContentRegionAvail().x * 0.25f; // Make the line 25% of the width
        const float handle_center_x = (handle_min.x + handle_max.x) / 2.0f;

        ImU32 color;
        if (ImGui::IsItemActive()) {
            color = ImGui::GetColorU32(ImGuiCol_ButtonActive);
        } else if (ImGui::IsItemHovered()) {
            color = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        } else {
            color = ImGui::GetColorU32(ImGuiCol_Button);
        }

        draw_list->AddLine(
            ImVec2(handle_center_x - handle_width / 2, handle_line_y),
            ImVec2(handle_center_x + handle_width / 2, handle_line_y),
            color,
            2.0f
        );

        ImGui::PopID();
        return changed;
    }
}
