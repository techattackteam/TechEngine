#include "UIInspector.hpp"


#include "ui/Widget.hpp"
#include "imgui_stdlib.h"
#include "core/Logger.hpp"
#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"

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

            // Use a collapsing header for good organization
            if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::PushID("Position"); // Avoid ID clashes with other inspectors

                // --- ANCHOR PRESET DROPDOWN ---
                // An array of names corresponding to your AnchorPreset enum
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
                    glm::vec4 rootScreenRect = {0.0f, 0.0f, (float)m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer().m_screenWidth, (float)m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer().m_screenHeight};
                    glm::vec4 parentScreenRect = m_selectedWidget->m_parent ? m_selectedWidget->m_parent->m_finalScreenRect : rootScreenRect;
                    m_selectedWidget->changeAnchor(static_cast<Widget::AnchorPreset>(currentPreset), parentScreenRect);

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
                // This is the core of the dynamic UI. We check if the anchors are stretched on each axis.
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
                glm::vec4 rootFinalScreenRect = {0.0f, 0.0f, (float)m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer().m_screenWidth, (float)m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer().m_screenHeight};
                m_selectedWidget->calculateLayout(m_selectedWidget->m_parent ? m_selectedWidget->m_parent->m_finalScreenRect : rootFinalScreenRect);
            }
        }


        if (!m_selectedWidget) {
            return;
        }
        auto& properties = m_selectedWidget->getProperties();
        for (auto& property: properties) {
            ImGui::PushID(property.name.c_str());
            ImGui::Text("%s", property.name.c_str());

            if (property.type == "color") {
                /*//Rml::Colourb currentColor = m_selectedWidget->m_rmlElement->GetProperty<Rml::Colourb>(property.rcssProperty);

                float colorsFloat[4] = {
                    static_cast<float>(currentColor.red) / 255.f,
                    static_cast<float>(currentColor.green) / 255.f,
                    static_cast<float>(currentColor.blue) / 255.f,
                    static_cast<float>(currentColor.alpha) / 255.f
                };
                if (ImGui::ColorEdit4("##ColorValue", colorsFloat)) {
                    std::stringstream ss;
                    ss << '#' << std::hex << std::setfill('0')
                            << std::setw(2) << std::lround(colorsFloat[0] * 255.0f)
                            << std::setw(2) << std::lround(colorsFloat[1] * 255.0f)
                            << std::setw(2) << std::lround(colorsFloat[2] * 255.0f)
                            << std::setw(2) << std::lround(colorsFloat[3] * 255.0f);
                    std::string color = ss.str();

                    TE_LOGGER_INFO("Setting color property '{0}' to '{1}'", property.name, color);
                    //m_selectedWidget->m_rmlElement->SetProperty(property.rcssProperty, color);
                    //property.onChange(color);
                }*/
            } else if (property.type == "string" /*&& property.target == "text-content"*/) {
                //std::string text;
                //Rml::ElementText* targetElement = dynamic_cast<Rml::ElementText*>(m_selectedWidget->m_rmlElement);
                //targetElement = m_selectedWidget->m_rmlElement->GetElementById("label");
                //std::string current = targetElement->GetText();
                //TE_LOGGER_INFO("Current text content: {}", current.c_str());
                //if (ImGui::InputTextWithHint("Label", current.c_str(), &text)) {
                //    //property.onChange(text);
                //    TE_LOGGER_INFO("Setting text property '{0}' to '{1}'", property.name, text);
                //}
            }
            ImGui::PopID();
        }
    }

    void UIInspector::setSelectedWidget(std::shared_ptr<Widget> widget) {
        this->m_selectedWidget = widget;
    }
}
