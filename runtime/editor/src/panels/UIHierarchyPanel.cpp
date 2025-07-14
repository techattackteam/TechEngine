#include "UIHierarchyPanel.hpp"

#include "UIEditor.hpp"
#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"

#include <RmlUi/Core/ElementDocument.h>

namespace TechEngine {
    UIHierarchyPanel::UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry) : Panel(editorSystemsRegistry), m_appSystemsRegistry(appSystemsRegistry) {
    }

    void UIHierarchyPanel::onInit() {
        m_context = m_appSystemsRegistry.getSystem<Renderer>().getUIContext();
    }

    void UIHierarchyPanel::onUpdate() {
        m_isWidgetHovered = false;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f)); // Adjust spacing between items
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f)); // Compact frame padding
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f); // Reduce indentation

        for (int i = 0; i < m_context->GetNumDocuments(); ++i) {
            Rml::ElementDocument* doc = m_context->GetDocument(i);
            if (!doc) {
                continue;
            }
            for (int child_idx = 0; child_idx < doc->GetNumChildren(); ++child_idx) {
                drawWidgetNode(doc->GetChild(child_idx));
            }
        }
        ImGui::PopStyleVar(3);

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_editor->setSelectedWidget(nullptr);
        }

        if (!m_isWidgetHovered && ImGui::BeginPopupContextWindow("Widget Creation", 1)) {
            if (ImGui::BeginMenu("Base Widgets")) {
                for (const Widget& widget: m_editor->getWidgetsRegistry().getBaseWidgets()) {
                    if (ImGui::MenuItem(widget.getName().c_str())) {
                        ImGui::Text("%s", widget.getName().c_str());
                        m_editor->createWidget(nullptr, widget.getName(), true);
                    }
                }
                ImGui::EndMenu();
            }
            for (const Widget& widget: m_editor->getWidgetsRegistry().getWidgets()) {
                if (ImGui::MenuItem(widget.getName().c_str())) {
                    ImGui::Text("%s", widget.getName().c_str());
                    m_editor->createWidget(nullptr, widget.getName(), false);
                }
            }
            ImGui::EndPopup();
        }

        if (!m_widgetsToDelete.empty()) {
            for (const std::shared_ptr<Widget>& widget: m_widgetsToDelete) {
                m_editor->deleteWidget(widget);
                if (m_editor->getSelectedWidget() == widget) {
                    m_editor->setSelectedWidget(nullptr);
                }
            }
            m_widgetsToDelete.clear();

            m_widgetsOrder.clear();
        }
    }

    void UIHierarchyPanel::setEditor(UIEditor* editor) {
        m_editor = editor;
    }

    void UIHierarchyPanel::drawWidgetNode(Rml::Element* element) {
        if (!element) return;

        std::shared_ptr<Widget> widget = nullptr;
        auto it = m_editor->m_elementToWidgetMap.find(element);
        if (it != m_editor->m_elementToWidgetMap.end()) {
            widget = it->second;
        }

        std::string label = element->GetTagName();
        if (!element->GetId().empty()) {
            label += "#" + element->GetId();
        }
        if (widget) {
            label = widget->getName() + " (" + label + ")";
        }

        bool isSelected = m_editor->m_selectedWidget == widget;
        ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_Leaf;

        if (element->GetNumChildren() == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        ImGui::PushID((void*)(intptr_t)element);
        bool opened = ImGui::TreeNodeEx("##TreeNode", flags, "%s", label.c_str());
        m_isWidgetHovered |= ImGui::IsItemHovered();
        ImGui::PopID();
        if (ImGui::IsItemClicked() && widget) {
            m_editor->setSelectedWidget(widget);
        }

        if (ImGui::BeginPopupContextItem()) {
            m_editor->setSelectedWidget(widget);
            if (ImGui::MenuItem("Delete Widget")) {
                m_widgetsToDelete.push_back(widget);
            }
            ImGui::EndPopup();
        }


        if (opened) {
            for (int i = 0; i < element->GetNumChildren(); ++i) {
                drawWidgetNode(element->GetChild(i));
            }
            ImGui::TreePop();
        }
    }
}
