#include "UIEditor.hpp"

#include "renderer/Renderer.hpp"
#include "sceneView.hpp"
#include "ui/ContainerWidget.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/TextWidget.hpp"

#include <imgui_internal.h>

#include "ui/InputTextWidget.hpp"
#include "ui/InteractableWidget.hpp"

namespace TechEngine {
    UIEditor::UIEditor(SystemsRegistry& editorSystemsRegistry,
                       SystemsRegistry& appSystemsRegistry,
                       GameView& gameView) : DockPanel(editorSystemsRegistry),
                                             m_uiView(editorSystemsRegistry, appSystemsRegistry, this),
                                             m_uiHierarchy(editorSystemsRegistry, appSystemsRegistry),
                                             m_uiInspector(editorSystemsRegistry, appSystemsRegistry), m_gameView(gameView),
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

    /*
    std::shared_ptr<Widget> UIEditor::createWidget(const std::shared_ptr<Widget>& parent, const std::string& type, const std::string& name) {

    }
    */

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
}
