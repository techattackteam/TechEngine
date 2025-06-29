#include "UIInspector.hpp"

#include <RmlUi/Core/ElementDocument.h>

#include "renderer/ui/widget/Canvas.hpp"
#include "imgui_stdlib.h"

namespace TechEngine {
    UIInspector::UIInspector(SystemsRegistry& editorSystemsRegistry) : Panel(editorSystemsRegistry) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        m_styleVars.emplace_back(ImGuiStyleVar_WindowBorderSize, 0.0f);
        m_styleVars.emplace_back(ImGuiStyleVar_WindowRounding, 0.0f);
    }

    void UIInspector::onInit() {
    }

    void UIInspector::onUpdate() {
        /*if (m_selectedWidget) {
            ImGui::Text("Type: %s", "Panel");
            ImGui::Separator();
            Canvas* panel = dynamic_cast<Canvas*>(m_selectedWidget);
            // Alignment Inspector
        

            // Position Inspector
            if (ImGui::DragFloat2("Position (px)", &panel->m_position.x, 1.0f, 0.0f, 10000.0f, "%.1f")) {
                panel->m_rmlElement->SetProperty(Rml::PropertyId::Left, Rml::Property(panel->m_position.x, Rml::Unit::PX));
                panel->m_rmlElement->SetProperty(Rml::PropertyId::Top, Rml::Property(panel->m_position.y, Rml::Unit::PX));
            }

            // Size Inspector
            if (ImGui::DragFloat2("Size (px)", &panel->m_size.x)) {
                panel->m_rmlElement->SetProperty(Rml::PropertyId::Width, Rml::Property(panel->m_size.x, Rml::Unit::PX));
                panel->m_rmlElement->SetProperty(Rml::PropertyId::Height, Rml::Property(panel->m_size.y, Rml::Unit::PX));
            }


            float color[4] = {panel->m_backgroundColor.red / 255.f, panel->m_backgroundColor.green / 255.f, panel->m_backgroundColor.blue / 255.f, panel->m_backgroundColor.alpha / 255.f};
            if (ImGui::ColorEdit4("Background Color", color)) {
                panel->m_backgroundColor = Rml::Colourb(color[0] * 255, color[1] * 255, color[2] * 255, color[3] * 255);
                panel->m_rmlElement->SetProperty(Rml::PropertyId::BackgroundColor, Rml::Property(panel->m_backgroundColor, Rml::Unit::COLOUR));
            }
        }*/

        if (!m_selectedWidget) {
            return;
        }
        auto& properties = m_selectedWidget->getProperties();
        for (auto& property: properties) {
            ImGui::PushID(property.name.c_str());
            ImGui::Text("%s", property.name.c_str());
            switch (property.type) {
                case PropertyType::Vector2f: {
                    auto& value = std::get<Rml::Vector2f>(property.value);
                    if (ImGui::DragFloat2("##value", &value.x, 0.1f)) {
                        property.onChange(value);
                    }
                    break;
                }
                case PropertyType::Vector3f: {
                    auto& value = std::get<Rml::Vector3f>(property.value);
                    if (ImGui::DragFloat3("##value", &value.x, 0.1f)) {
                        property.onChange(value);
                    }
                    break;
                }
                case PropertyType::String: {
                    std::string& value = std::get<std::string>(property.value);
                    if (ImGui::InputText("##value", &value)) {
                        property.onChange(value);
                    }
                    break;
                }
                case PropertyType::Color: {
                    auto& value = std::get<Rml::Colourb>(property.value);
                    float color[4] = {static_cast<float>(value.red) / 255.f, static_cast<float>(value.green) / 255.f, static_cast<float>(value.blue) / 255.f, static_cast<float>(value.alpha) / 255.f};
                    if (ImGui::ColorEdit4("##value", color)) {
                        property.onChange(Rml::Colourb(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255), static_cast<int>(color[3] * 255)));
                    }
                    value = Rml::Colourb(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255), static_cast<int>(color[3] * 255));
                    break;
                }
                case PropertyType::Bool: {
                    bool& value = std::get<bool>(property.value);
                    if (ImGui::Checkbox("##value", &value)) {
                        property.onChange(value);
                    }
                    break;
                }
                case PropertyType::Int: {
                    int& value = std::get<int>(property.value);
                    if (ImGui::DragInt("##value", &value)) {
                        property.onChange(value);
                    }
                    break;
                }
                case PropertyType::Float: {
                    float& value = std::get<float>(property.value);
                    if (ImGui::DragFloat("##value", &value)) {
                        property.onChange(value);
                    }
                    break;
                }
                case PropertyType::Enum: {
                    auto& current_index = std::get<int>(property.value);
                    const char* current_item_label = property.enumOptions[current_index].c_str();

                    if (ImGui::BeginCombo(property.name.c_str(), current_item_label)) {
                        for (int i = 0; i < property.enumOptions.size(); ++i) {
                            const bool is_selected = (current_index == i);
                            if (ImGui::Selectable(property.enumOptions[i].c_str(), is_selected)) {
                                current_index = i; // Update the variant's value
                                if (property.onChange) property.onChange(property.value);
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                break;
                default:
                    ImGui::Text("Unsupported property type: %d", static_cast<int>(property.type));
            }
            ImGui::PopID();
        }
    }

    void UIInspector::setSelectedWidget(Widget* widget) {
        this->m_selectedWidget = widget;
    }
}
