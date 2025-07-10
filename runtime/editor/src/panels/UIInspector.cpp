#include "UIInspector.hpp"

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>

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
            ImGui::Text("Widget: %s", m_selectedWidget->getName().c_str());
            ImGui::Separator();

            // Alignment Inspector
            bool positionChanged = ImGui::DragFloat2("Center Position (px)", &m_selectedWidget->m_position.x, 1.0f, 0.0f, 10000.0f, "%.1f");
            bool sizeChanged = ImGui::DragFloat2("Size (px)", &m_selectedWidget->m_size.x, 1.0f, 1.0f, 10000.0f, "%.1f"); // Added min value to avoid zero/negative 
            if (positionChanged || sizeChanged) {
                auto* element = m_selectedWidget->m_rmlElement;

                const Rml::Vector2f& centerPos = m_selectedWidget->m_position;
                const Rml::Vector2f& size = m_selectedWidget->m_size;

                /*
                float left = centerPos.x - (size.x / 2.0f);
                float top = centerPos.y - (size.y / 2.0f);
                */

                float left = centerPos.x;
                float top = centerPos.y;

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

            if (property.type == "color") {
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
                    m_selectedWidget->m_rmlElement->SetProperty(property.rcssProperty, color);
                    property.onChange(color);
                }
            } else if (property.type == "string" /*&& property.target == "text-content"*/) {
                std::string text;
                Rml::ElementText* targetElement = dynamic_cast<Rml::ElementText*>(m_selectedWidget->m_rmlElement);
                //targetElement = m_selectedWidget->m_rmlElement->GetElementById("label");
                std::string current = targetElement->GetText();
                TE_LOGGER_INFO("Current text content: {}", current.c_str());
                if (ImGui::InputTextWithHint("Label", current.c_str(), &text)) {
                    //property.onChange(text);
                    TE_LOGGER_INFO("Setting text property '{0}' to '{1}'", property.name, text);
                }
            }
            ImGui::PopID();
        }
    }

    void UIInspector::setSelectedWidget(std::shared_ptr<Widget> widget) {
        this->m_selectedWidget = widget;
    }
}
