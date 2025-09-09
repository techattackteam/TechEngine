#include "WidgetsRegistry.hpp"
#include "core/Logger.hpp"
#include "json.hpp"

#include <fstream>
#include <memory>

#include "ContainerWidget.hpp"
#include "InputTextWidget.hpp"
#include "PanelWidget.hpp"
#include "TextWidget.hpp"
#include "core/Timer.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "input/Input.hpp"
#include "input/Mouse.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/Widget.hpp"


namespace TechEngine {
    WidgetsRegistry::WidgetsRegistry(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void WidgetsRegistry::init() {
        m_widgetsTemplates.push_back(ContainerWidget());
        m_widgetsTemplates.push_back(PanelWidget());
        m_widgetsTemplates.push_back(TextWidget());
        m_widgetsTemplates.push_back(InputTextWidget(m_systemsRegistry));
        //m_widgetsTemplates.push_back(ButtonWidget());
        //m_widgetsTemplates.push_back(ImageWidget());
        loadJson(R"(C:\dev\TechEngine\bin\runtime\editor\debug\New Project\resources\client\assets\ui\widgets.json)");

        m_systemsRegistry.getSystem<EventDispatcher>().subscribe<KeyPressedEvent>([this](const std::shared_ptr<Event>& event) {
            auto* keyPressedEvent = dynamic_cast<KeyPressedEvent*>(event.get());
            if (keyPressedEvent->getKey().getKeyCode() >= MOUSE_1 && keyPressedEvent->getKey().getKeyCode() <= MOUSE_8) {
                onMousePressedEvent(std::dynamic_pointer_cast<KeyPressedEvent>(event));
            } else {
                onKeyPressedEvent(std::dynamic_pointer_cast<KeyPressedEvent>(event));
            }
        });
    }

    void WidgetsRegistry::onUpdate() {
        for (auto& widget: m_widgets) {
            if (widget) {
                widget->update(m_systemsRegistry.getSystem<Timer>().getDeltaTime());
            }
        }
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

    void WidgetsRegistry::onMousePressedEvent(const std::shared_ptr<KeyPressedEvent>& event) {
        std::shared_ptr<Widget> clickedWidget = nullptr;
        glm::vec2 mousePosition = m_systemsRegistry.getSystem<Input>().getMouse().getPosition(); // This is window related position
        for (auto& widget: m_widgets) {
            // Check if the widget is an InputTextWidget and if the click is inside its bounds
            if (auto inputField = std::dynamic_pointer_cast<InputTextWidget>(widget)) {
                const auto& rect = inputField->getFinalScreenRect(); // This is just the game view in the editor
                if (mousePosition.x >= rect.x && mousePosition.x <= rect.x + rect.z &&
                    mousePosition.y >= rect.y && mousePosition.y <= rect.y + rect.w) {
                    clickedWidget = inputField;
                    break; // Found the top-most widget
                }
            }
        }

        // --- FOCUS MANAGEMENT ---
        // If we clicked a new widget
        if (clickedWidget && m_focusedWidget != clickedWidget) {
            // Lose focus on the old widget, if any
            if (auto oldFocused = m_focusedWidget) {
                if (auto oldInput = std::dynamic_pointer_cast<InputTextWidget>(oldFocused)) {
                    oldInput->loseFocus();
                }
            }

            // Gain focus on the new widget
            std::dynamic_pointer_cast<InputTextWidget>(clickedWidget)->gainFocus();
            m_focusedWidget = clickedWidget;
        }
        // If we clicked outside of any interactable widget
        else if (!clickedWidget) {
            if (auto oldFocused = m_focusedWidget) {
                if (auto oldInput = std::dynamic_pointer_cast<InputTextWidget>(oldFocused)) {
                    oldInput->loseFocus();
                }
            }
            m_focusedWidget.reset();
        }
    }

    void WidgetsRegistry::onKeyPressedEvent(const std::shared_ptr<KeyPressedEvent>& event) {
        if (auto focused = m_focusedWidget) {
            if (auto input = std::dynamic_pointer_cast<InputTextWidget>(focused)) {
                input->onKeyPressed(event->getKey());
            }
        }
    }
}
