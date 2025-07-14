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

    void UIHierarchyPanel::reorderWidget(const std::shared_ptr<Widget>& widget, int new_index) {
        Rml::Element* element = widget->getRmlElement();
        Rml::Element* parent = element->GetParentNode();

        if (!parent)
            return;

        // Remove first
        Rml::ElementPtr elementPtr = parent->RemoveChild(element);

        // Clamp new_index
        new_index = std::clamp(new_index, 0, parent->GetNumChildren());

        // Reinsert at new index
        if (new_index == parent->GetNumChildren()) {
            parent->AppendChild(std::move(elementPtr));
        } else {
            Rml::Element* sibling = parent->GetChild(new_index);
            parent->InsertBefore(std::move(elementPtr), sibling);
        }

        m_widgetsOrder.clear();
        for (int i = 0; i < parent->GetNumChildren(); ++i) {
            Rml::Element* child = parent->GetChild(i);
            auto it = m_editor->m_elementToWidgetMap.find(child);
            if (it != m_editor->m_elementToWidgetMap.end()) {
                m_widgetsOrder.push_back(it->second);
            }
        }
    }

    void UIHierarchyPanel::reparentWidget(const std::shared_ptr<Widget>& childWidget, const std::shared_ptr<Widget>& newParentWidget) {
        if (!childWidget || !newParentWidget) return;

        Rml::Element* childElement = childWidget->getRmlElement();
        Rml::Element* newParentElement = newParentWidget->getRmlElement();
        if (!childElement || !newParentElement) return;

        Rml::Element* oldParent = childElement->GetParentNode();
        if (oldParent == newParentElement) return;

        // Avoid circular parenting
        Rml::Element* cursor = newParentElement;
        while (cursor) {
            if (cursor == childElement) return;
            cursor = cursor->GetParentNode();
        }

        const Rml::Vector2f worldPosBefore = childElement->GetAbsoluteOffset();
        const Rml::Vector2f sizeBefore = childElement->GetBox().GetSize(Rml::BoxArea::Content);

        Rml::ElementPtr elementPtr = oldParent->RemoveChild(childElement);
        newParentElement->AppendChild(std::move(elementPtr));

        const Rml::Vector2f parentWorldPos = newParentElement->GetAbsoluteOffset();

        const Rml::Vector2f newLocalPos = worldPosBefore - parentWorldPos;

        const Rml::Vector2f anchorMin = childWidget->m_anchorMin;
        const Rml::Vector2f anchorMax = childWidget->m_anchorMax;
        const Rml::Vector2f pivot = childWidget->m_pivot;

        Rml::Vector2f parentSize = newParentElement->GetBox().GetSize(Rml::BoxArea::Content);
        Rml::Vector2f anchorPoint = {
            parentSize.x * (anchorMin.x + anchorMax.x) * 0.5f,
            parentSize.y * (anchorMin.y + anchorMax.y) * 0.5f
        };

        Rml::Vector2f finalPos = newLocalPos - anchorPoint + Rml::Vector2f(pivot.x * sizeBefore.x, pivot.y * sizeBefore.y);

        childWidget->m_anchoredPosition = finalPos;

        childElement->SetProperty("left", std::to_string(finalPos.x) + "px");
        childElement->SetProperty("top", std::to_string(finalPos.y) + "px");

        childWidget->applyStyles(childElement, newParentElement);
        TE_LOGGER_INFO("Reparented widget '%s' under '%s' preserving layout", childWidget->getName().c_str(), newParentWidget->getName().c_str());
    }


    void UIHierarchyPanel::drawWidgetNode(Rml::Element* element) {
        if (!element) return;
        ImGuiStyle& style = ImGui::GetStyle();
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

        const float frameHeight = ImGui::GetFrameHeight();
        const float totalItemHeight = frameHeight + style.ItemSpacing.y;
        const ImVec2 initialCursorPos = ImGui::GetCursorPos();

        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, totalItemHeight));

        enum class DropAction { None, Reparent, Reorder };
        DropAction dropAction = DropAction::Reparent;

        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            const ImRect targetRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            const ImVec2 mousePos = ImGui::GetMousePos();

            ImRect reparentRect = targetRect;
            reparentRect.Min.y += 4; // 4px top reorder zone
            reparentRect.Max.y -= 4; // 4px bottom reorder zone

            if (payload && payload->DataSize == sizeof(std::shared_ptr<Widget>)) {
                std::shared_ptr<Widget> draggedWidget = *static_cast<std::shared_ptr<Widget>*>(payload->Data);

                bool isSameWidget = false;
                if (draggedWidget == widget) {
                    isSameWidget = true;
                }

                if (!reparentRect.Contains(mousePos)) {
                    dropAction = DropAction::Reorder;
                }

                if (!isSameWidget && dropAction == DropAction::Reorder) {
                    auto originalWidget = std::find(m_widgetsOrder.begin(), m_widgetsOrder.end(), draggedWidget);
                    auto targetWidget = std::find(m_widgetsOrder.begin(), m_widgetsOrder.end(), widget);
                    if (originalWidget != m_widgetsOrder.end() && targetWidget != m_widgetsOrder.end()) {
                        bool isDroppingAbove = (mousePos.y < reparentRect.Min.y);
                        if (isDroppingAbove) {
                            if (std::next(originalWidget) == targetWidget) {
                                isSameWidget = true;
                            }
                        } else {
                            if (targetWidget != m_widgetsOrder.begin() && std::next(targetWidget) == originalWidget) {
                                isSameWidget = true;
                            }
                        }
                    }
                }

                if (!isSameWidget) {
                    ImU32 feedbackColor = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
                    if (dropAction == DropAction::Reorder) {
                        float lineY = (mousePos.y < reparentRect.Min.y) ? targetRect.Min.y : targetRect.Max.y;
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(targetRect.Min.x, lineY),
                                                            ImVec2(targetRect.Max.x, lineY),
                                                            feedbackColor, 2.0f);
                    } else {
                        ImVec2 visualNodeMin = targetRect.Min;
                        ImVec2 visualNodeMax = ImVec2(targetRect.Max.x, targetRect.Min.y + frameHeight);
                        ImGui::GetWindowDrawList()->AddRect(visualNodeMin, visualNodeMax, feedbackColor, 2.0f, 0, 2.0f);
                    }
                }
            }

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                "WIDGET_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                std::shared_ptr<Widget> draggedWidget = *static_cast<std::shared_ptr<Widget>*>(payload->Data);
                if (draggedWidget != widget) {
                    int index = std::ranges::find(m_widgetsOrder, widget) - m_widgetsOrder.begin();

                    if (dropAction == DropAction::Reorder) {
                        int newIndex = (mousePos.y < reparentRect.Min.y) ? index : index + 1;
                        reorderWidget(draggedWidget, newIndex);
                        TE_LOGGER_INFO("Reordering widget {0} to index {1}", draggedWidget->getName(), newIndex);
                    } else {
                        reparentWidget(draggedWidget, widget);
                        TE_LOGGER_INFO("Parenting entity {0} under entity {1}", draggedWidget->getName(), widget->getName());
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGui::SetCursorPos(initialCursorPos);

        const float textHeight = ImGui::GetTextLineHeight();
        const float verticalPadding = (frameHeight - textHeight) * 0.5f;
        if (verticalPadding > 0) {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + verticalPadding);
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

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("WIDGET_DRAG", &widget, sizeof(std::shared_ptr<Widget>));
            ImGui::Text("%s", widget ? widget->getName().c_str() : label.c_str());
            ImGui::EndDragDropSource();
        }

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
