#include "WidgetsRegistry.hpp"
#include "core/Logger.hpp"
#include "json.hpp"

#include <fstream>
#include <memory>

#include "ContainerWidget.hpp"
#include "PanelWidget.hpp"
#include "TextWidget.hpp"
#include "ui/Widget.hpp"


namespace TechEngine {
    WidgetsRegistry::WidgetsRegistry() {
    }

    void WidgetsRegistry::init() {
        m_widgetsTemplates.push_back(ContainerWidget());
        m_widgetsTemplates.push_back(TextWidget());
        m_widgetsTemplates.push_back(PanelWidget());
        loadJson(R"(C:\dev\TechEngine\bin\runtime\editor\debug\New Project\resources\client\assets\ui\widgets.json)");
    }

    bool WidgetsRegistry::loadJson(const std::string& jsonFile) {
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
        return true;
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

    const std::vector<Widget>& WidgetsRegistry::getWidgetsTemplates() const {
        return m_widgetsTemplates;
    }

    std::vector<std::shared_ptr<Widget>>& WidgetsRegistry::getWidgets() {
        return m_widgets;
    }
}
