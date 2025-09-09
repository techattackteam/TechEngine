#include "UIHierarchyPanel.hpp"

#include "UIEditor.hpp"
#include "systems/SystemsRegistry.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    UIHierarchyPanel::UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry)
        : Panel(editorSystemsRegistry), m_appSystemsRegistry(appSystemsRegistry) {
    }

    void UIHierarchyPanel::onInit() {
        // We will populate m_rootWidgetsOrder when the editor is set and widgets are loaded.
    }


    void UIHierarchyPanel::onUpdate() {
        if (m_isHierarchyDirty) {
            rebuildDisplayList();
        }

        m_isWidgetHovered = false;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f)); // Adjust spacing between items
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f)); // Compact frame padding
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f); // Reduce indentation
        for (auto& node: m_displayList) {
            ImGui::PushID(node.widget.get());
            drawWidgetNode(node);
            ImGui::PopID();
        }
        ImGui::PopStyleVar(3);
        ImGui::Dummy(ImGui::GetContentRegionAvail());

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_editor->setSelectedWidget(nullptr);
        }

        //TODO: Change this for an action instead changing the panel internal state on the fly
        if (!m_isWidgetHovered && ImGui::BeginPopupContextWindow("Widget Creation", 1)) {
            ImGui::Text("BaseWidgets");
            if (ImGui::MenuItem("Container")) {
                m_editor->createWidget(nullptr, "Container", "Container");
                setHierarchyDirty();
            } else if (ImGui::MenuItem("Panel")) {
                m_editor->createWidget(nullptr, "Panel", "Panel");
                setHierarchyDirty();
            } else if (ImGui::MenuItem("Text")) {
                m_editor->createWidget(nullptr, "Text", "Text");
                setHierarchyDirty();
            } else if (ImGui::MenuItem("InputText")) {
                m_editor->createWidget(nullptr, "InputText", "InputText");
                setHierarchyDirty();
            } /* else if (ImGui::MenuItem("Button")) {
                m_editor->createWidget(nullptr, "Button", "Button");
            } else if (ImGui::MenuItem("Image")) {
                m_editor->createWidget(nullptr, "Image", "Image");
            }*/
            ImGui::Separator();
            ImGui::Text("Widgets");
            //for (const Widget& widget: m_editor->getWidgetsRegistry().getWidgetsTemplates()) {
            //    if (ImGui::MenuItem(widget.getName().c_str())) {
            //        ImGui::Text("%s", widget.getName().c_str());
            //        m_editor->createWidget(nullptr, widget.getName(), false);
            //    }
            //}
            ImGui::EndPopup();
        }

        if (!m_pendingActions.empty()) {
            applyPendingActions();
            m_pendingActions.clear();
        }
    }

    void UIHierarchyPanel::setEditor(UIEditor* editor) {
        m_editor = editor;
    }

    void UIHierarchyPanel::setHierarchyDirty() {
        m_isHierarchyDirty = true;
    }

    void UIHierarchyPanel::rebuildDisplayList() {
        m_displayList.clear();
        for (const std::shared_ptr<Widget>& widget: m_editor->getWidgetsRegistry().getWidgets()) {
            if (widget && widget->m_parent == nullptr) {
                recursiveAddToDisplayList(widget, 0);
            }
        }
        m_isHierarchyDirty = false;
    }

    void UIHierarchyPanel::recursiveAddToDisplayList(const std::shared_ptr<Widget>& widget, int depth) {
        m_displayList.push_back({widget, depth});

        if (widget->m_isHierarchyOpen) {
            for (const auto& child: widget->m_children) {
                recursiveAddToDisplayList(child, depth + 1);
            }
        }
    }

    void UIHierarchyPanel::applyPendingActions() {
        for (const auto& action: m_pendingActions) {
            switch (action.type) {
                case ActionType::Move:
                    reparentWidget(action.subject, action.destination, action.index);
                    break;
            }
        }
    }

    //This should change for a internal class instead of the UI
    void UIHierarchyPanel::reparentWidget(const std::shared_ptr<Widget>& widgetToMove, const std::shared_ptr<Widget>& newParent, int newIndex) {
        if (widgetToMove->m_parent != nullptr) {
            widgetToMove->m_parent->removeChild(widgetToMove);
        }
        if (newParent) {
            // If the new parent is valid, add the widget to its children
            newParent->addChild(widgetToMove, newIndex);
        } else {
            // If the new parent is null, we are moving to the root level
            std::vector<std::shared_ptr<Widget>>& children = m_editor->getWidgetsRegistry().getWidgets();
            auto it = std::remove(children.begin(), children.end(), widgetToMove);
            if (it != children.end()) {
                children.erase(it, children.end());
            }
            children.insert(children.begin() + newIndex, widgetToMove);
        }
    }

    void UIHierarchyPanel::drawWidgetNode(HierarchyNode& node) {
        if (!node.widget) return;

        bool isFirst = (node.depth == 0 && m_displayList.begin() == std::find(m_displayList.begin(), m_displayList.end(), node));

        ImGui::PushID(node.widget.get());

        // Apply indentation based on the node's depth in the hierarchy
        ImGui::Indent((node.depth + 1) * ImGui::GetStyle().IndentSpacing);

        // --- Setup TreeNode ---
        bool isSelected = m_editor->getSelectedWidget() == node.widget;
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (isSelected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (node.widget->m_children.empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        // --- Draw the actual node ---
        bool opened = ImGui::TreeNodeEx("##TreeNode", flags, "%s", node.widget->getName().c_str());
        m_isWidgetHovered |= ImGui::IsItemHovered();

        // --- Handle State Changes ---
        if (ImGui::IsItemClicked(0) && !ImGui::IsItemToggledOpen()) {
            m_editor->setSelectedWidget(node.widget);
        }

        // If the open/close state changes, update the widget and rebuild the list next frame
        if (ImGui::IsItemToggledOpen()) {
            node.widget->m_isHierarchyOpen = opened;
            setHierarchyDirty();
        }

        // --- Drag and Drop Source ---
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("WIDGET_DRAG", &node.widget, sizeof(std::shared_ptr<Widget>));
            ImGui::Text("%s", node.widget->getName().c_str());
            ImGui::EndDragDropSource();
        }

        // --- Drag and Drop Target ---
        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            if (payload && payload->IsDataType("WIDGET_DRAG") && !node.widget->hasChild(*static_cast<std::shared_ptr<Widget>*>(payload->Data))) {
                auto draggedWidget = *static_cast<std::shared_ptr<Widget>*>(payload->Data);

                // Determine drop action: Reparent vs. Reorder
                enum class DropAction { Reparent, Reorder_After, Reorder_Before };
                DropAction dropAction = DropAction::Reparent;
                float dropZoneHeight = ImGui::GetItemRectSize().y * 0.35f;
                ImVec2 dropZoneMin = ImGui::GetItemRectMin();
                ImVec2 dropZoneMax = ImGui::GetItemRectMax();
                ImVec2 mousePos = ImGui::GetMousePos();
                if (mousePos.y < dropZoneMin.y + dropZoneHeight && isFirst) {
                    dropAction = DropAction::Reorder_Before;
                } else if (mousePos.y > dropZoneMax.y - dropZoneHeight) {
                    dropAction = DropAction::Reorder_After;
                }

                // Draw visual feedback
                ImU32 feedbackColor = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
                if (dropAction == DropAction::Reparent) {
                    ImGui::GetWindowDrawList()->AddRect(dropZoneMin, dropZoneMax, feedbackColor);
                } else {
                    float lineY = (dropAction == DropAction::Reorder_Before && isFirst) ? dropZoneMin.y : dropZoneMax.y;
                    ImGui::GetWindowDrawList()->AddLine({dropZoneMin.x, lineY}, {dropZoneMax.x, lineY}, feedbackColor, 2.5f);
                }

                if (ImGui::AcceptDragDropPayload("WIDGET_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                    std::shared_ptr<Widget> newParent = nullptr;
                    int newIndex = -1;

                    if (dropAction == DropAction::Reparent) {
                        newParent = node.widget;
                        newIndex = newParent->m_children.size();
                    } else { // Reordering
                        newParent = node.widget->m_parent;

                        // Find the index of the node we are dropping ON
                        int targetIndex = 0;
                        if (newParent) {
                            std::vector<std::shared_ptr<Widget>>& siblings = newParent->m_children;
                            auto it = std::find(siblings.begin(), siblings.end(), node.widget);
                            targetIndex = std::distance(siblings.begin(), it);
                        } else {
                            auto it = std::find(m_displayList.begin(), m_displayList.end(), node.widget);
                            targetIndex = std::distance(m_displayList.begin(), it);
                        }

                        auto it = std::find(m_displayList.begin(), m_displayList.end(), draggedWidget);
                        auto itOther = std::find(m_displayList.begin(), m_displayList.end(), node.widget);
                        // Hackish solution to handle when the dragging and widget from the a lower to higher index it doesn't skip an index
                        newIndex = (dropAction == DropAction::Reorder_Before && isFirst) ? targetIndex : (itOther > it ? targetIndex : targetIndex + 1);
                    }

                    // CRITICAL: Instead of modifying the tree directly, we queue an action.
                    m_pendingActions.push_back({ActionType::Move, draggedWidget, newParent, newIndex});
                    setHierarchyDirty();
                }
            }
            ImGui::EndDragDropTarget();
        }

        // --- Right-Click Context Menu ---
        if (ImGui::BeginPopupContextItem()) {
            m_editor->setSelectedWidget(node.widget); // Select the widget when right-clicking

            std::string nameBuffer = node.widget->getName();
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "##Rename", nameBuffer)) {
                node.widget->rename(nameBuffer);
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Delete")) {
                //// Queue a "Delete" action (future enhancement) or use a temporary delete list.
                //// For now, let's assume a delete list processed in applyPendingActions.
                //m_widgetsToDelete.push_back(node.widget);
                //setHierarchyDirty();
                //ImGui::CloseCurrentPopup();
                TE_LOGGER_WARN("UIHierarchyPanel: Delete action is not implemented yet. Please implement deletion logic.");
            }
            ImGui::EndPopup();
        }

        // If the node was opened, we no longer need to recurse. The flat list handles it.
        // We just need to pop the tree node.
        if
        (opened) {
            ImGui::TreePop();
        }

        ImGui::Unindent((node.depth + 1) * ImGui::GetStyle().IndentSpacing);
        ImGui::PopID();
    }
}
