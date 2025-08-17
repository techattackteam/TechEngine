#include "UIEditor.hpp"

#include "renderer/Renderer.hpp"
#include "sceneView.hpp"
#include "ui/ContainerWidget.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/TextWidget.hpp"

#include <imgui_internal.h>

namespace TechEngine {
    UIEditor::UIEditor(SystemsRegistry& editorSystemsRegistry,
                       SystemsRegistry& appSystemsRegistry,
                       GameView& gameView) : DockPanel(editorSystemsRegistry),
                                             m_uiView(editorSystemsRegistry, appSystemsRegistry, this),
                                             m_uiHierarchy(editorSystemsRegistry, appSystemsRegistry),
                                             m_uiInspector(editorSystemsRegistry), m_gameView(gameView),
                                             m_appSystemsRegistry(appSystemsRegistry) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }

    void UIEditor::onInit() {
        m_uiHierarchy.setEditor(this);
        m_uiHierarchy.init("UI Hierarchy", &m_dockSpaceWindowClass);
        m_uiView.init("UI View", &m_dockSpaceWindowClass);
        m_uiInspector.init("UI Inspector", &m_dockSpaceWindowClass);
        //m_widgetsRegistry.loadJson("C:\\dev\\TechEngine\\bin\\runtime\\editor\\debug\\New Project\\resources\\client\\assets\\ui\\baseWidgets.json", true);
    }

    void UIEditor::onUpdate() {
        m_uiHierarchy.update();
        m_uiInspector.update();
        m_uiView.update();
    }

    std::shared_ptr<Widget> UIEditor::createWidget(const std::shared_ptr<Widget>& parent, const std::string& type, const std::string& name) {
        std::shared_ptr<Widget> widget;
        if (type == "Container") {
            widget = std::make_shared<ContainerWidget>();
        } else if (type == "Panel") {
            widget = std::make_shared<PanelWidget>();
        } else if (type == "Text") {
            widget = std::make_shared<TextWidget>();
        } /*else if (type == "Button") {
            widget = std::make_shared<ButtonWidget>();
        } else if (type == "Image") {
            widget = std::make_shared<ImageWidget>();
        }*/
        widget->m_parent = parent;
        widget->rename(name);
        getWidgetsRegistry().getWidgets().emplace_back(widget);
        return widget;
    }

    bool UIEditor::deleteWidget(const std::shared_ptr<Widget>& widget) {
        return false;
        /*if (!widget || !widget->getRmlElement()) {
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
        }*/
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
}
