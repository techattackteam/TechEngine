#include "WidgetsRegistry.hpp"
#include "core/Logger.hpp"
#include "json.hpp"

#include <fstream>
#include <memory>

#include "ui/Widget.hpp"


namespace TechEngine {
    bool WidgetsRegistry::loadJson(const std::string& jsonFile, bool base) {
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
        m_widgetsTemplates.clear();
        if (base) {
            for (const auto& widgetData: widgets_json) {
                Widget widget;
                widget.m_name = widgetData.at("name").get<std::string>();
                widget.m_category = widgetData.at("category").get<std::string>();
                widget.m_description = widgetData.at("description").get<std::string>();
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
                        property.defaultValue = "";
                    }

                    widget.m_properties.push_back(property);
                }
                m_baseWidgetsTemplates.push_back(widget);
            }
        } else {
            for (const auto& widgetData: widgets_json) {
                Widget widget;
                widget.m_name = widgetData.at("name").get<std::string>();
                widget.m_category = widgetData.at("category").get<std::string>();
                widget.m_description = widgetData.at("description").get<std::string>();
                const auto& children = widgetData.at("children");
                if (!children.is_array()) {
                    TE_LOGGER_ERROR("Properties for widget '{}' are not an array in file: {}", widget.m_name.c_str(), jsonFile.c_str());
                    continue;
                }
                for (const auto& child: children) {
                    std::string childType = child.at("type").get<std::string>();
                    widget.m_childrenTypes.push_back(childType);
                }
                m_widgetsTemplates.push_back(widget);
            }
        }
        return true;
    }

    std::shared_ptr<Widget> WidgetsRegistry::createBaseWidget(const std::string& name) {
        Widget* widget = nullptr;
        for (auto& w: m_baseWidgetsTemplates) {
            if (w.getName() == name) {
                widget = &w;
                break;
            }
        }
        if (widget) {
            return std::make_shared<Widget>(*widget);
        }
        TE_LOGGER_ERROR("Widget with name '{0}' not found in registry.", name.c_str());
        return nullptr;
    }

    std::shared_ptr<Widget> WidgetsRegistry::createWidget(const std::string& name) {
        Widget* widget = nullptr;
        for (auto& w: m_widgetsTemplates) {
            if (w.getName() == name) {
                widget = &w;
                break;
            }
        }
        if (widget) {
            return std::make_shared<Widget>(*widget);
        }
        TE_LOGGER_ERROR("Widget with name '{0}' not found in registry.", name.c_str());
        return nullptr;
    }

    const std::vector<Widget> WidgetsRegistry::getBaseWidgets() const {
        return m_baseWidgetsTemplates;
    }
}
