#include "WidgetsRegistry.hpp"
#include "core/Logger.hpp"
#include "json.hpp"

#include <fstream>
#include <RmlUi/Core/Factory.h>

#include "ui/Widget.hpp"


namespace TechEngine {
    bool WidgetsRegistry::load(const std::string& jsonFile) {
        std::ifstream stream(jsonFile);
        if (!stream.is_open()) {
            TE_LOGGER_CRITICAL("Failed to open widgets registry file: {}", jsonFile.c_str());
            return false;
        }
        nlohmann::json data;

        try {
            data = nlohmann::json::parse(stream);
        } catch (const nlohmann::json::parse_error& e) {
            TE_LOGGER_ERROR("Failed to parse widgets registry file: {}. Error: {}", jsonFile.c_str(), e.what());
            return false;
        }
        stream.close();
        const auto& widgets_json = data["widgets"];
        if (widgets_json.is_null() || !widgets_json.is_array()) {
            TE_LOGGER_ERROR("Widgets registry file is empty or not an array: {}", jsonFile.c_str());
            return false;
        }
        m_widgets.clear();
        for (const auto& widgetData: widgets_json) {
            Widget widget;
            widget.m_name = widgetData.at("name").get<std::string>();
            widget.m_category = widgetData.at("category").get<std::string>();
            widget.m_description = widgetData.at("description").get<std::string>();
            widget.m_rmlSnippet = widgetData.at("rml_snippet").get<std::string>();
            const auto& properties = widgetData.at("properties");
            if (!properties.is_array()) {
                TE_LOGGER_ERROR("Properties for widget '{}' are not an array in file: {}", widget.m_name.c_str(), jsonFile.c_str());
                continue;
            }
            for (const auto& prop: properties) {
                WidgetProperty property;
                property.name = prop.at("name").get<std::string>();
                property.rcssProperty = prop.at("rcss_property").get<std::string>();
                property.type = prop.at("type").get<std::string>();
                if (prop.contains("default")) {
                    property.defaultValue = prop.at("default").get<std::string>();
                } else {
                    property.defaultValue = ""; // Default to empty string if not provided
                }

                widget.m_properties.push_back(property);
            }
            m_widgets.push_back(widget);
        }
        return true;
    }

    Widget* WidgetsRegistry::createWidget(const std::string& name) {
        Widget* widget = nullptr;
        for (auto& w: m_widgets) {
            if (w.getName() == name) {
                widget = &w;
                break;
            }
        }
        // Copy the widget properties to a new instance
        if (widget) {
            Widget* newWidget = new Widget(*widget);
            // Initialize the RmlUi element for the new widget
            Rml::ElementPtr element = Rml::Factory::InstanceElement(nullptr, "div", widget->m_rmlSnippet, Rml::XMLAttributes());
            if (element) {
                newWidget->setRmlElement(element.get());
                for (auto& property: newWidget->m_properties) {
                    property.onChange = [&property, newWidget, &element](const WidgetProperty::ProperTyValue& value) {
                        if (property.type == "color") {
                            //element->SetProperty(Rml::PropertyId::BackgroundColor, Rml::Property(std::get<Rml::Colourb>(value), Rml::Unit::COLOUR));
                            TE_LOGGER_INFO("Changing color property '{0}' for widget '{1}'", property.name.c_str(), newWidget->getName().c_str());
                        }
                    };
                }
                return newWidget;
            } else {
                TE_LOGGER_ERROR("Failed to create RmlUi element for widget: {}", name.c_str());
                delete newWidget; // Clean up if element creation failed
            }
        } else {
            TE_LOGGER_ERROR("Widget with name '{}' not found in registry.", name.c_str());
        }
        return nullptr;
    }
}
