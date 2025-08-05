#include "UIEditor.hpp"

#include <imgui_internal.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/Factory.h>

#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    UIEditor::UIEditor(SystemsRegistry& editorSystemsRegistry,
                       SystemsRegistry& appSystemsRegistry) : DockPanel(editorSystemsRegistry),
                                                              m_uiView(editorSystemsRegistry, appSystemsRegistry, this),
                                                              m_uiHierarchy(editorSystemsRegistry, appSystemsRegistry),
                                                              m_uiInspector(editorSystemsRegistry),
                                                              m_appSystemsRegistry(appSystemsRegistry),
                                                              m_context(nullptr) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }

    void UIEditor::onInit() {
        m_context = m_appSystemsRegistry.getSystem<Renderer>().getUIContext();
        if (m_context) {
            Rml::ElementDocument* doc = m_context->GetDocument(0);
            if (!doc) {
                TE_LOGGER_ERROR("UIEditor: No RmlUi document found in the context.");
                return;
            }
        }
        m_document = m_context->GetDocument(0);
        m_uiHierarchy.setEditor(this);
        m_uiHierarchy.init("UI Hierarchy", &m_dockSpaceWindowClass);
        m_uiView.init("UI View", &m_dockSpaceWindowClass);
        m_uiInspector.init("UI Inspector", &m_dockSpaceWindowClass);
        m_widgetsRegistry.loadJson("C:\\dev\\TechEngine\\bin\\runtime\\editor\\debug\\New Project\\resources\\client\\assets\\ui\\baseWidgets.json", true);
        m_widgetsRegistry.loadJson("C:\\dev\\TechEngine\\bin\\runtime\\editor\\debug\\New Project\\resources\\client\\assets\\ui\\widgets.json", false);
    }

    void UIEditor::onUpdate() {
        m_uiHierarchy.update();
        m_uiInspector.update();
        m_uiView.update();
    }

    std::shared_ptr<Widget> UIEditor::createWidget(Rml::Element* parent, const std::string& name, bool base) {
        std::shared_ptr<Widget> widget; // Assuming this function creates a widget and returns a pointer to it

        Rml::ElementPtr elementPtr;
        if (base) {
            if (name == "Label") {
                //elementPtr = Rml::Factory::InstanceElement(parent, "div", "div", Rml::XMLAttributes());
                //elementPtr->SetProperty("position", "absolute");
                //elementPtr->SetProperty("align-self", "center");
                //elementPtr->SetProperty("transform-origin-x", "center");
                //elementPtr->SetProperty("transform-origin-y", "center");
                //elementPtr->SetProperty("font-size", "18px");
                //elementPtr->SetProperty("font-weight", "bold");
                //elementPtr->SetProperty("font-family", "Lato");

                //auto text = Rml::Factory::InstanceElement(parent, "#text", "text", Rml::XMLAttributes());
                //dynamic_cast<Rml::ElementText*>(elementPtr.get())->SetText("Hello World!");
                //elementPtr->AppendChild(std::move(text));
                elementPtr = Rml::Factory::InstanceElement(nullptr, "div", "div", Rml::XMLAttributes());
                elementPtr->SetProperty("position", "absolute");
                elementPtr->SetProperty("align-self", "center");
                elementPtr->SetProperty("transform-origin-x", "center");
                elementPtr->SetProperty("transform-origin-y", "center");
                elementPtr->SetProperty("background-color", "#ffffffff");
                elementPtr->SetProperty("color", "#0000ffff");
                elementPtr->SetProperty("font-size", "18px");
                elementPtr->SetProperty("font-weight", "bold");
                elementPtr->SetProperty("font-family", "Lato");
                elementPtr->SetInnerRML("Hello World!");
            } else {
                elementPtr = Rml::Factory::InstanceElement(parent, "div", "div", Rml::XMLAttributes());
                elementPtr->SetProperty("position", "absolute");
                elementPtr->SetProperty("align-self", "center");
                elementPtr->SetProperty("transform-origin-x", "center");
                elementPtr->SetProperty("transform-origin-y", "center");
                elementPtr->SetProperty("background-color", "#ffffffff");
                elementPtr->SetProperty("color", "#0000ffff");
                elementPtr->SetProperty("font-size", "18px");
                elementPtr->SetProperty("font-weight", "bold");
                elementPtr->SetProperty("font-family", "Lato");


                /*auto it = m_elementToWidgetMap.begin();
                auto otherElement = it != m_elementToWidgetMap.end() ? it->first : nullptr;
                while (otherElement->GetNumChildren() > 0) {
                    Rml::Element* child = otherElement->GetChild(0);
                    elementPtr->AppendChild(otherElement->RemoveChild(child));
                }
                m_context->Update();*/
            }
        } else {
            elementPtr = Rml::Factory::InstanceElement(parent, "div", "div", Rml::XMLAttributes());
            elementPtr->SetProperty("position", "absolute"); // Set the ID of the element to the widget's name
            elementPtr->SetProperty("display", "flex");
            elementPtr->SetProperty("justify-content", "center"); // horizontal
            elementPtr->SetProperty("align-items", "center"); // vertical
        }

        elementPtr->SetProperty("width", "100%");
        elementPtr->SetProperty("height", "100%");
        Rml::Element* element = elementPtr.get();
        if (!elementPtr) {
            //Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to create Rml::Element for widget: %s", newWidget->getRmlTag());
            return nullptr; // Handle error if element creation fails
        }

        if (base) {
            widget = m_widgetsRegistry.createBaseWidget(name);
        } else {
            widget = m_widgetsRegistry.createWidget(name);
            for (const auto& childType: widget->m_childrenTypes) {
                std::shared_ptr<Widget> childWidget = createWidget(element, childType, true); // Create child widget
                if (childWidget) {
                    widget->m_children.push_back(childWidget);
                } else {
                    TE_LOGGER_ERROR("Failed to create child widget of type: {0}", childType.c_str());
                }
            }
        }

        widget->setRmlElement(elementPtr.get());
        if (parent != nullptr) {
            element = parent->AppendChild(std::move(elementPtr));
        } else {
            element = m_document->AppendChild(std::move(elementPtr));
        }

        widget->setRmlElement(element);
        for (auto& property: widget->getProperties()) {
            property.onChange = [this, &property, widget](const std::string& value) {
                widget->m_rmlElement->SetProperty(property.rcssProperty, value);
            };
        }
        m_elementToWidgetMap[element] = widget;
        std::vector<WidgetProperty> properties = widget->getProperties();
        for (auto& prop: properties) {
            if (prop.type == "int") {
                prop.onChange(prop.defaultValue); // Convert string to int
            } else if (prop.type == "float") {
                prop.onChange(prop.defaultValue);
            } else if (prop.type == "bool") {
                prop.onChange(prop.defaultValue); // Convert string to bool
            } else if (prop.type == "string") {
                //dynamic_cast<Rml::ElementText*>(element)->SetText(prop.defaultValue);
            } else if (prop.type == "vector2f") {
                Rml::Vector2f vec;
                sscanf_s(prop.defaultValue.c_str(), "%f,%f", &vec.x, &vec.y);
                //prop.onChange(vec);
            } else if (prop.type == "vector3f") {
                Rml::Vector3f vec;
                sscanf_s(prop.defaultValue.c_str(), "%f,%f,%f", &vec.x, &vec.y, &vec.z);
                //prop.onChange(vec);
            } else if (prop.type == "color") {
                std::stringstream ss;
                ss << '#' << std::hex << std::setfill('0')
                        << std::setw(2) << std::lround(255)
                        << std::setw(2) << std::lround(255)
                        << std::setw(2) << std::lround(255)
                        << std::setw(2) << std::lround(255);
                std::string color = ss.str();
                //prop.onChange(color);
            }
        }


        widget->applyStyles(element, parent);
        m_context->Update();
        TE_LOGGER_INFO("Element Top: {0}", element->GetAbsoluteTop());
        return widget;
    }

    bool UIEditor::deleteWidget(const std::shared_ptr<Widget>& widget) {
        if (!widget || !widget->getRmlElement()) {
            TE_LOGGER_ERROR("UIEditor: Cannot delete widget, widget or its Rml element is null.");
            return false;
        }
        Rml::Element* element = widget->getRmlElement();
        Rml::Element* parent = element->GetParentNode();
        if (m_elementToWidgetMap.find(parent) == m_elementToWidgetMap.end()) {
            element->GetParentNode()->RemoveChild(element);
            m_elementToWidgetMap.erase(element);
            TE_LOGGER_INFO("UIEditor: Widget '{0}' deleted successfully.", widget->getName());
            return true;
        } else {
            std::shared_ptr<Widget> parentWidget = m_elementToWidgetMap[parent];
            if (parentWidget) {
                auto it = std::find(parentWidget->m_children.begin(), parentWidget->m_children.end(), widget);
                if (it != parentWidget->m_children.end()) {
                    parentWidget->m_children.erase(it);
                    m_elementToWidgetMap.erase(element);
                    TE_LOGGER_INFO("UIEditor: Widget '{0}' deleted successfully.", widget->getName());
                    return true;
                } else {
                    TE_LOGGER_ERROR("UIEditor: Widget '{0}' not found in parent's children.", widget->getName());
                    return false;
                }
            } else {
                TE_LOGGER_ERROR("UIEditor: Parent widget not found for widget '{0}'.", widget->getName());
                return false;
            }
        }
    }

    void UIEditor::setSelectedWidget(const std::shared_ptr<Widget>& widget) {
        m_selectedWidget = widget;
        m_uiInspector.setSelectedWidget(widget);
    }

    void UIEditor::setupInitialDockingLayout() {
        ImGui::DockBuilderRemoveNode(m_dockSpaceID);
        ImGui::DockBuilderAddNode(m_dockSpaceID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(m_dockSpaceID, ImGui::GetMainViewport()->Size);

        ImGuiID dockMainID = m_dockSpaceID;
        ImGuiID dockRightID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Right, 0.2f, nullptr, &dockMainID);
        ImGuiID dockLeftID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Left, 0.2f, nullptr, &dockMainID);

        ImGui::DockBuilderDockWindow((m_uiView.getName() + "##" + std::to_string(m_uiView.getId())).c_str(),
                                     dockMainID);
        ImGui::DockBuilderDockWindow(
            (m_uiHierarchy.getName() + "##" + std::to_string(m_uiHierarchy.getId())).c_str(),
            dockLeftID);
        ImGui::DockBuilderDockWindow(
            (m_uiInspector.getName() + "##" + std::to_string(m_uiInspector.getId())).c_str(), dockRightID);

        ImGui::DockBuilderFinish(m_dockSpaceID);
    }


    void UIEditor::loadRmlDocument(const std::string& path) {
        if (!m_context) {
            TE_LOGGER_ERROR("Cannot load RML document: UI Context is null.");
            return;
        }
        //m_context->UnloadAllDocuments(); // Unload previous document if any
        //Rml::ElementDocument* doc = m_context->LoadDocument(path);
        //if (doc) {
        //    doc->Show();
        //    // doc->RemoveReference(); // Context owns it
        //    m_currentRmlDocumentPath = path;
        //    TE_LOGGER_INFO("InteractiveUIEditorPanel: Loaded RML document '%s' into editor context.", path.c_str());
        //} else {
        //    TE_LOGGER_ERROR("InteractiveUIEditorPanel: Failed to load RML document '%s'.", path.c_str());
        //    m_currentRmlDocumentPath.clear();
        //}
    }

    /*void UIEditor::drawRmlElementInHierarchy(Rml::Element* element) {
        if (!element) return;

        // Find our C++ widget counterpart for this element
        std::shared_ptr<Widget> widget = nullptr;
        auto it = m_elementToWidgetMap.find(element);
        if (it != m_elementToWidgetMap.end()) {
            widget = it->second;
        }

        // Determine the label for the ImGui node
        std::string label = element->GetTagName();
        if (!element->GetId().empty()) {
            label += "#" + element->GetId();
        }
        if (widget) {
            // If we have a C++ widget, use its name for a friendlier display
            label = widget->getName() + " (" + label + ")";
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (m_selectedWidget == widget) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        // Make leaf nodes (like text) not have an arrow
        if (element->GetNumChildren() == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        bool node_open = ImGui::TreeNodeEx((void*)element, flags, "%s", label.c_str());

        if (ImGui::IsItemClicked() && widget) {
            setSelectedWidget(widget);
        }

        // Add a right-click menu to create children
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Add Panel")) {
                //createWidget<Canvas>(element, "Panel"); // Create a child for the currently right-clicked Rml::Element
            }
            ImGui::EndPopup();
        }


        if (node_open) {
            for (int i = 0; i < element->GetNumChildren(); ++i) {
                drawRmlElementInHierarchy(element->GetChild(i));
            }
            ImGui::TreePop();
        }
    }*/

    const std::unordered_map<Rml::Element*, std::shared_ptr<Widget>>& UIEditor::getElementToWidgetMap() {
        return m_elementToWidgetMap;
    }
}
