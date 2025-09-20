#include "WidgetsRegistry.hpp"
#include "core/Logger.hpp"

#include <fstream>
#include <memory>
#include <stack>

#include "ContainerWidget.hpp"
#include "InputTextWidget.hpp"
#include "InteractableWidget.hpp"
#include "PanelWidget.hpp"
#include "TextWidget.hpp"
#include "core/Timer.hpp"
#include "systems/SystemsRegistry.hpp"
#include "input/Input.hpp"
#include "input/Mouse.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"
#include "renderer/OldRenderer.hpp"
#include "scene/ScenesManager.hpp"
#include "utils/YAMLUtils.hpp"

namespace TechEngine {
    WidgetsRegistry::WidgetsRegistry(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void WidgetsRegistry::init() {
        //createDefaultWidgetYamlFile();
        loadYaml(R"(C:\dev\TechEngine\bin\runtime\editor\debug\New Project\resources\client\assets\ui\widgets.yaml)");

        m_systemsRegistry.getSystem<EventDispatcher>().subscribe<KeyPressedEvent>([this](const std::shared_ptr<Event>& event) {
            auto* keyPressedEvent = dynamic_cast<KeyPressedEvent*>(event.get());
            if (keyPressedEvent->getKey().getKeyCode() >= MOUSE_1 && keyPressedEvent->getKey().getKeyCode() <= MOUSE_8) {
                onMousePressedEvent(std::dynamic_pointer_cast<KeyPressedEvent>(event));
            } else {
                onKeyPressedEvent(std::dynamic_pointer_cast<KeyPressedEvent>(event));
            }
        });

        m_systemsRegistry.getSystem<EventDispatcher>().subscribe<MouseMoveEvent>([this](const std::shared_ptr<Event>& event) {
            onMouseMoveEvent(std::dynamic_pointer_cast<MouseMoveEvent>(event));
        });
        m_serializer.init(m_systemsRegistry);
        std::string sceneName = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().getName();
        m_serializer.deserializeUI(sceneName, m_systemsRegistry);
    }

    void WidgetsRegistry::onUpdate() {
        for (auto& widget: m_widgets) {
            if (widget) {
                if (widget->m_isDirty) {
                    calculateWidgetLayout(widget);
                }
                widget->update(m_systemsRegistry.getSystem<Timer>().getDeltaTime());
            }
        }
    }

    bool WidgetsRegistry::loadYaml(const std::string& yamlFile) {
        try {
            YAML::Node config = YAML::LoadFile(yamlFile);
            const YAML::Node& widgetsList = config["Widgets"];
            if (!widgetsList || !widgetsList.IsSequence()) {
                return false; // Error: "Widgets" key not found or not a sequence
            }

            m_widgetsTemplates.resize(widgetsList.size());
            std::stack<std::pair<YAML::Node, CustomWidgetTemplate&>> nodeStack;

            // --- 1. Initial Seeding of the Stack ---
            for (int i = 0; i < widgetsList.size(); i++) {
                const YAML::Node& topLevelNode = widgetsList[i];
                CustomWidgetTemplate& rootWidget = m_widgetsTemplates[i];

                // Parse its properties
                if (topLevelNode["name"]) {
                    rootWidget.name = topLevelNode["name"].as<std::string>();
                }
                if (topLevelNode["category"]) {
                    rootWidget.category = topLevelNode["category"].as<std::string>();
                }
                if (topLevelNode["description"]) {
                    rootWidget.description = topLevelNode["description"].as<std::string>();
                }

                // If this top-level widget has children, push them onto the stack to be processed.
                const YAML::Node& childrenNode = topLevelNode["children"];
                if (childrenNode && childrenNode.IsSequence()) {
                    // Push children in reverse order so they are popped in the correct order
                    for (size_t i = childrenNode.size(); i > 0; i--) {
                        nodeStack.emplace(childrenNode[i - 1], rootWidget);
                    }
                }
            }

            // --- 2. Main Processing Loop (Iterative DFS) ---
            while (!nodeStack.empty()) {
                // Pop the next node to process
                auto [currentNode, parentWidget] = nodeStack.top();
                nodeStack.pop();

                // Create the new child widget within its parent's m_children vector
                CustomWidgetTemplate& newWidget = parentWidget.children.emplace_back();
                if (currentNode["type"]) {
                    newWidget.type = currentNode["type"].as<std::string>();
                }
                // Parse the new widget's properties
                if (currentNode["name"]) {
                    newWidget.name = currentNode["name"].as<std::string>();
                    //TODO: Confirm that the Widget exists
                }

                // If this new widget has children of its own, add them to the stack.
                const YAML::Node& childrenNode = currentNode["children"];
                if (childrenNode && childrenNode.IsSequence()) {
                    // Push children in reverse order to maintain original order when processed
                    for (size_t i = childrenNode.size(); i > 0; i--) {
                        nodeStack.emplace(childrenNode[i - 1], newWidget);
                    }
                }
            }
        } catch (const YAML::Exception& e) {
            TE_LOGGER_ERROR("Failed to load widgets from YAML file: {0}", e.what());
            return false;
        }
        return true;
    }

    std::shared_ptr<Widget> WidgetsRegistry::createWidget(const std::shared_ptr<Widget>& parent, const std::string& name, const std::string& type, bool custom) {
        std::shared_ptr<Widget> widget = custom ? createCustomWidget(m_widgetsTemplates[0], name) : createBaseWidget(type, name);
        if (!widget) {
            TE_LOGGER_CRITICAL("WidgetsRegistry::createWidget: Failed to create widget of type '{0}'", name.c_str());
            return nullptr;
        }
        if (parent) {
            parent->addChild(widget, parent->m_children.size());
        } else {
            m_rootWidgets.emplace_back(widget);
        }
        widget->rename(name);
        UIRenderer& uiRenderer = m_systemsRegistry.getSystem<OldRenderer>().getUIRenderer();
        widget->calculateLayout(
            parent
                ? parent->m_finalScreenRect
                : glm::vec4(0.0f, 0.0f, (float)uiRenderer.m_screenWidth, (float)uiRenderer.m_screenHeight), uiRenderer.getDpiScale());
        rebuildFlattenedList();
        return widget;
    }

    void WidgetsRegistry::calculateWidgetLayout(const std::shared_ptr<Widget>& widget) {
        UIRenderer& uiRenderer = m_systemsRegistry.getSystem<OldRenderer>().getUIRenderer();
        glm::vec4 rootFinalScreenRect = {0.0f, 0.0f, (float)uiRenderer.m_screenWidth, (float)uiRenderer.m_screenHeight};
        widget->calculateLayout(widget->m_parent ? widget->m_parent->m_finalScreenRect : rootFinalScreenRect, uiRenderer.getDpiScale());
        widget->m_isDirty = false;
    }

    void WidgetsRegistry::queueCommand(std::unique_ptr<ICommand> command) {
        m_commandQueue.push_back(std::move(command));
    }

    std::vector<std::shared_ptr<Widget>>& WidgetsRegistry::getWidgets() {
        return m_widgets;
    }


    const std::vector<WidgetsRegistry::CustomWidgetTemplate>& WidgetsRegistry::getWidgetTemplates() {
        return m_widgetsTemplates;
    }

    const std::vector<std::shared_ptr<Widget>>& WidgetsRegistry::getRootWidgets() const {
        return m_rootWidgets;
    }

    WidgetsSerializer& WidgetsRegistry::getSerializer() {
        return m_serializer;
    }

    void WidgetsRegistry::applyCommands() {
        if (m_commandQueue.empty()) {
            return;
        }

        for (const auto& command: m_commandQueue) {
            command->execute();
        }
        m_commandQueue.clear();

        rebuildFlattenedList();
    }

    void WidgetsRegistry::performWidgetMove(const std::shared_ptr<Widget>& widgetToMove, const std::shared_ptr<Widget>& newParent, int newSiblingIndex) {
        if (auto oldParent = widgetToMove->m_parent) {
            widgetToMove->m_parent->removeChild(widgetToMove);
        } else {
            std::erase(m_rootWidgets, widgetToMove);
        }

        // 2. Add to new parent at the correct sibling index
        if (newParent) {
            newParent->addChild(widgetToMove, newSiblingIndex);
        } else {
            if (newSiblingIndex < 0 || newSiblingIndex > m_rootWidgets.size()) {
                newSiblingIndex = m_rootWidgets.size();
            }
            m_rootWidgets.insert(m_rootWidgets.begin() + newSiblingIndex, widgetToMove);
            widgetToMove->m_parent = nullptr;
        }
    }

    void WidgetsRegistry::rebuildFlattenedList() {
        m_widgets.clear();
        std::function<void(const std::shared_ptr<Widget>&)> traverse;
        traverse = [&](const std::shared_ptr<Widget>& widget) {
            m_widgets.push_back(widget);
            for (const auto& child: widget->m_children) {
                traverse(child);
            }
        };

        for (const auto& root: m_rootWidgets) {
            traverse(root);
        }
    }

    void WidgetsRegistry::createDefaultWidgetYamlFile() {
        std::string path = R"(C:\dev\TechEngine\bin\runtime\editor\debug\New Project\resources\client\assets\ui\widgets.yaml)";
        std::ofstream file(path);
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Widgets" << YAML::Value << YAML::BeginSeq;

        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << "Button";
        out << YAML::Key << "category" << YAML::Value << "Interactable";
        out << YAML::Key << "description" << YAML::Value << "Interactable Button";

        out << YAML::Key << "children" << YAML::Value << YAML::BeginSeq;

        out << YAML::BeginMap;
        out << YAML::Key << "type" << YAML::Value << "Interactable";
        out << YAML::Key << "name" << YAML::Value << "Interactable";
        out << YAML::EndMap;
        out << YAML::BeginMap;
        out << YAML::Key << "type" << YAML::Value << "Text";
        out << YAML::Key << "name" << YAML::Value << "Label";
        out << YAML::EndMap;
        out << YAML::BeginMap;
        out << YAML::Key << "type" << YAML::Value << "Panel";
        out << YAML::Key << "name" << YAML::Value << "Background";
        out << YAML::EndMap;

        out << YAML::EndSeq;

        out << YAML::EndMap;

        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << "Text Field";

        out << YAML::Key << "category" << YAML::Value << "Interactable";
        out << YAML::Key << "description" << YAML::Value << "Interactable Button";

        out << YAML::Key << "children" << YAML::Value << YAML::BeginSeq;

        out << YAML::BeginMap;
        out << YAML::Key << "type" << YAML::Value << "InputText";
        out << YAML::Key << "name" << YAML::Value << "Text";
        out << YAML::EndMap;
        out << YAML::BeginMap;
        out << YAML::Key << "type" << YAML::Value << "Panel";
        out << YAML::Key << "name" << YAML::Value << "Background";
        out << YAML::EndMap;


        out << YAML::EndMap;
        out << YAML::EndSeq;
        out << YAML::EndMap;
        //out << YAML::Key << "name" << YAML::Value << "Test";
        //out << YAML::Key << "category" << YAML::Value << "TestCategory";
        //out << YAML::Key << "description" << YAML::Value << "This is a test widget";
        //out << YAML::Key << "children" << YAML::Value << YAML::BeginMap;

        //out << YAML::Key << "Container" << YAML::Value << YAML::BeginMap;
        //out << YAML::Key << "name" << YAML::Value << "Name for the widget Container";
        //out << YAML::EndMap;
        //
        //out << YAML::Key << "Panel" << YAML::Value << YAML::BeginMap;
        //out << YAML::Key << "name" << YAML::Value << "Name for the widget Panel";
        //out << YAML::EndMap;
        //
        //out << YAML::EndMap;
        //
        //out << YAML::EndMap;
        //out << YAML::EndMap;

        //out << YAML::Key << "Another Test" << YAML::Value << YAML::BeginMap;
        //out << YAML::Key << "name" << YAML::Value << "Another Test";
        //out << YAML::Key << "category" << YAML::Value << "TestCategory";
        //out << YAML::Key << "description" << YAML::Value << "This is another test widget";


        file << out.c_str();
    }

    void WidgetsRegistry::onMouseMoveEvent(const std::shared_ptr<MouseMoveEvent>& event) {
        EventDispatcher& eventDispatcher = m_systemsRegistry.getSystem<EventDispatcher>();
        glm::vec2 mousePosition = event->getToPosition();
        bool found = false;
        for (auto& widget: m_widgets) {
            const auto& rect = widget->getFinalScreenRect();
            if (mousePosition.x >= rect.x && mousePosition.x <= rect.x + rect.z &&
                mousePosition.y >= rect.y && mousePosition.y <= rect.y + rect.w) {
                if (m_currentlyHoveredWidget != widget) {
                    if (m_currentlyHoveredWidget) {
                        m_currentlyHoveredWidget->onMouseLeftRect(eventDispatcher);
                    }
                    if (widget) {
                        widget->onMouseEnteredRect(eventDispatcher);
                    }
                    m_currentlyHoveredWidget = widget;
                }
                found = true;
                break;
            }
        }
        if (!found && m_currentlyHoveredWidget) {
            m_currentlyHoveredWidget->onMouseLeftRect(eventDispatcher);
            m_currentlyHoveredWidget.reset();
        }
    }

    void WidgetsRegistry::onMousePressedEvent(const std::shared_ptr<KeyPressedEvent>& event) {
        std::shared_ptr<Widget> clickedWidget = nullptr;
        glm::vec2 mousePosition = m_systemsRegistry.getSystem<Input>().getMouse().getPosition(); // This is window related position
        for (auto& widget: m_widgets) {
            // Check if the widget is an InputTextWidget and if the click is inside its bounds
            if (std::dynamic_pointer_cast<InputTextWidget>(widget) ||
                std::dynamic_pointer_cast<InteractableWidget>(widget)) {
                const auto& rect = widget->getFinalScreenRect(); // This is just the game view in the editor
                if (mousePosition.x >= rect.x && mousePosition.x <= rect.x + rect.z &&
                    mousePosition.y >= rect.y && mousePosition.y <= rect.y + rect.w) {
                    clickedWidget = widget;
                    break;
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

            if (std::dynamic_pointer_cast<InputTextWidget>(clickedWidget)) {
                std::dynamic_pointer_cast<InputTextWidget>(clickedWidget)->gainFocus();
                m_focusedWidget = clickedWidget;
            } else if (std::dynamic_pointer_cast<InteractableWidget>(clickedWidget)) {
                std::dynamic_pointer_cast<InteractableWidget>(clickedWidget)->onMouseClick(m_systemsRegistry.getSystem<EventDispatcher>());
                m_focusedWidget.reset();
            }
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

    std::shared_ptr<Widget> WidgetsRegistry::createBaseWidget(const std::string& type, const std::string& name) {
        std::shared_ptr<Widget> widget;
        if (type == "Widget") {
            widget = std::make_shared<Widget>();
        } else if (type == "Container") {
            widget = std::make_shared<ContainerWidget>();
        } else if (type == "Panel") {
            widget = std::make_shared<PanelWidget>();
        } else if (type == "Text") {
            widget = std::make_shared<TextWidget>();
        } else if (type == "InputText") {
            widget = std::make_shared<InputTextWidget>(m_systemsRegistry);
        } else if (type == "Interactable") {
            widget = std::make_shared<InteractableWidget>();
            /* else if (type == "Button") {
            widget = std::make_shared<ButtonWidget>();
            } else if (type == "Image") {
            widget = std::make_shared<ImageWidget>();
            }*/
        } else {
            TE_LOGGER_CRITICAL("UIEditor: Unknown widget type '{0}'. Cannot create widget.", name.c_str());
        }

        return widget;
    }

    std::shared_ptr<Widget> WidgetsRegistry::createCustomWidget(const CustomWidgetTemplate& widgetTemplate, const std::string& name) {
        std::shared_ptr<Widget> widget = std::make_shared<Widget>(name);
        for (int i = 0; i < widgetTemplate.children.size(); i++) {
            const CustomWidgetTemplate& childTemplate = widgetTemplate.children[i];
            auto childWidget = createBaseWidget(childTemplate.type, childTemplate.type);
            if (childWidget) {
                performWidgetMove(childWidget, widget, i);
            } else {
                TE_LOGGER_CRITICAL("WidgetsRegistry::createCustomWidget: Failed to create child widget of type '{0}'", childTemplate.name.c_str());
            }
        }
        return widget;
    }
}
