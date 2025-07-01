#include "UIInspector.hpp"

#include <RmlUi/Core/ElementDocument.h>
#include "ui/Widget.hpp"
#include "imgui_stdlib.h"
#include "core/Logger.hpp"

namespace TechEngine {
    UIInspector::UIInspector(SystemsRegistry& editorSystemsRegistry) : Panel(editorSystemsRegistry) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        m_styleVars.emplace_back(ImGuiStyleVar_WindowBorderSize, 0.0f);
        m_styleVars.emplace_back(ImGuiStyleVar_WindowRounding, 0.0f);
    }

    void UIInspector::onInit() {
    }

    void UIInspector::onUpdate() {
        if (m_selectedWidget) {
            ImGui::Text("Type: %s", "Panel");
            ImGui::Separator();

            // Alignment Inspector
            bool positionChanged = ImGui::DragFloat2("Center Position (px)", &m_selectedWidget->m_position.x, 1.0f, 0.0f, 10000.0f, "%.1f");
            bool sizeChanged = ImGui::DragFloat2("Size (px)", &m_selectedWidget->m_size.x, 1.0f, 1.0f, 10000.0f, "%.1f"); // Added min value to avoid zero/negative 
            if (positionChanged || sizeChanged) {
                auto* element = m_selectedWidget->m_rmlElement;

                // Get the center position and size from our widget wrapper
                const Rml::Vector2f& centerPos = m_selectedWidget->m_position;
                const Rml::Vector2f& size = m_selectedWidget->m_size;

                // Calculate the top-left corner for RML based on the center and size
                float left = centerPos.x - (size.x / 2.0f);
                float top = centerPos.y - (size.y / 2.0f);

                // Set all the relevant properties on the Rml element
                element->SetProperty(Rml::PropertyId::Left, Rml::Property(left, Rml::Unit::PX));
                element->SetProperty(Rml::PropertyId::Top, Rml::Property(top, Rml::Unit::PX));
                element->SetProperty(Rml::PropertyId::Width, Rml::Property(size.x, Rml::Unit::PX));
                element->SetProperty(Rml::PropertyId::Height, Rml::Property(size.y, Rml::Unit::PX));
            }
        }

        if (!m_selectedWidget) {
            return;
        }
        auto& properties = m_selectedWidget->getProperties();
        for (auto& property: properties) {
            ImGui::PushID(property.name.c_str());
            ImGui::Text("%s", property.name.c_str());

            // 3. Use the "type" to decide which ImGui widget to draw.
            // THIS IS THE DYNAMIC PART!
            if (property.type == "color") {
                // Get the property from the element, using the default from the definition
                Rml::Colourb currentColor = m_selectedWidget->m_rmlElement->GetProperty<Rml::Colourb>(property.rcssProperty);

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
                    m_selectedWidget->m_rmlElement->SetProperty(property.rcssProperty, color); // Set the color property
                }
                ImGui::PopID();
                break;
            }

            /*ImGui::PushID(property.name.c_str());
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
            */
        }
    }

    void UIInspector::setSelectedWidget(Widget* widget) {
        this->m_selectedWidget = widget;
    }
}
