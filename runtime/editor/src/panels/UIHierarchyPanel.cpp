#include "UIHierarchyPanel.hpp"

#include "UIEditor.hpp"
#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"
#include "UIUtils/ImGuiUtils.hpp"


namespace TechEngine {
    UIHierarchyPanel::UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry) : Panel(editorSystemsRegistry), m_appSystemsRegistry(appSystemsRegistry) {
    }

    void UIHierarchyPanel::onInit() {
    }

    void UIHierarchyPanel::onUpdate() {
        m_isWidgetHovered = false;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f)); // Adjust spacing between items
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f)); // Compact frame padding
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f); // Reduce indentation

        /*for (int i = 0; i < m_context->GetNumDocuments(); ++i) {
            Rml::ElementDocument* doc = m_context->GetDocument(i);
            if (!doc) {
                continue;
            }
            for (int child_idx = 0; child_idx < doc->GetNumChildren(); ++child_idx) {
                drawWidgetNode(doc->GetChild(child_idx));
            }
        }*/
        ImGui::PopStyleVar(3);

        ImGui::Dummy(ImGui::GetContentRegionAvail());

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

    void UIHierarchyPanel::moveWidget(const std::shared_ptr<Widget>& childWidget, Widget* newParent, int newIndex) {
        /*if (!childWidget || !newParent) return;
        Rml::Element* childElement = childWidget->getRmlElement();
        if (!childElement) return;
        Rml::Element* oldParent = childElement->GetParentNode();
        if (!oldParent) return;

        if (oldParent != newParent) {
            Rml::Element* cursor = newParent;
            while (cursor) {
                if (cursor == childElement) {
                    TE_LOGGER_WARN("Cannot reparent a widget to one of its own descendants.");
                    return;
                }
                cursor = cursor->GetParentNode();
            }
        }

        const Rml::Vector2f worldPosBefore = childElement->GetAbsoluteOffset();
        const Rml::Vector2f sizeBefore = childElement->GetBox().GetSize(Rml::BoxArea::Content);

        Rml::ElementPtr elementPtr = oldParent->RemoveChild(childElement);

        if (oldParent == newParent) {
            int oldIndex = -1;
            for (int i = 0; i < oldParent->GetNumChildren(); ++i) {
                if (oldParent->GetChild(i) == elementPtr.get()) {
                    oldIndex = i;
                    break;
                }
            }
            if (oldIndex != -1 && oldIndex < newIndex) {
                newIndex--;
            }
        }

        int clampedIndex = std::clamp(newIndex, 0, newParent->GetNumChildren());

        if (clampedIndex >= newParent->GetNumChildren()) {
            newParent->AppendChild(std::move(elementPtr));
        } else {
            Rml::Element* sibling = newParent->GetChild(clampedIndex);
            newParent->InsertBefore(std::move(elementPtr), sibling);
        }

        if (oldParent != newParent) {
            const Rml::Vector2f parentWorldPos = newParent->GetAbsoluteOffset();
            const Rml::Vector2f newLocalPos = worldPosBefore - parentWorldPos;
            const Rml::Vector2f anchorMin = childWidget->m_anchorMin;
            const Rml::Vector2f anchorMax = childWidget->m_anchorMax;
            const Rml::Vector2f pivot = childWidget->m_pivot;

            Rml::Vector2f parentSize = newParent->GetBox().GetSize(Rml::BoxArea::Content);
            Rml::Vector2f anchorPoint = {
                parentSize.x * (anchorMin.x + anchorMax.x) * 0.5f,
                parentSize.y * (anchorMin.y + anchorMax.y) * 0.5f
            };

            Rml::Vector2f finalPos = newLocalPos - anchorPoint + Rml::Vector2f(pivot.x * sizeBefore.x, pivot.y * sizeBefore.y);

            childWidget->m_anchoredPosition = finalPos;
            childElement->SetProperty("left", std::to_string(finalPos.x) + "px");
            childElement->SetProperty("top", std::to_string(finalPos.y) + "px");

            childWidget->applyStyles(childElement, newParent);
        }

        m_widgetsOrder.clear();*/
    }

    void UIHierarchyPanel::drawWidgetNode(Widget* element) {
        /*if (!element) return;
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
                    if (dropAction == DropAction::Reorder) {
                        Rml::Element* newParent = widget->getRmlElement()->GetParentNode();
                        if (newParent) {
                            // Find the index of the target widget within its parent
                            int targetIndex = 0;
                            for (int i = 0; i < newParent->GetNumChildren(); ++i) {
                                if (newParent->GetChild(i) == widget->getRmlElement()) {
                                    targetIndex = i;
                                    break;
                                }
                            }

                            int newIndex = (mousePos.y < reparentRect.Min.y) ? targetIndex : targetIndex + 1;

                            moveWidget(draggedWidget, newParent, newIndex);
                            TE_LOGGER_INFO("Moved widget '{0}'", draggedWidget->getName());
                        }
                    } else {
                        moveWidget(draggedWidget, widget->getRmlElement(), widget->getRmlElement()->GetNumChildren());
                        TE_LOGGER_INFO("Parented widget '{0}' under '{1}'", draggedWidget->getName(), widget->getName());
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
            std::string nameBuffer = widget ? widget->getName() : "";
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename Widget", "Rename", nameBuffer)) {
                widget->rename(nameBuffer);
            }
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
        }*/
    }
}
