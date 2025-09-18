#include "UIHierarchyPanel.hpp"

#include "UIEditor.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/commands/MoveWidgetCommand.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    UIHierarchyPanel::UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry)
        : Panel(editorSystemsRegistry), m_appSystemsRegistry(appSystemsRegistry) {
    }

    void UIHierarchyPanel::onInit() {
    }


    void UIHierarchyPanel::onUpdate() {
        if (m_isHierarchyDirty) {
            rebuildDisplayList();
        }

        m_isWidgetHovered = false;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 1.0f)); // Adjust spacing between items
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f)); // Compact frame padding
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f); // Reduce indentation
        for (auto& node: m_displayList) {
            ImGui::PushID(node.widget.get());
            drawDropZone(node.widget->m_parent, node.widget);
            drawWidgetNode(node);
            ImGui::PopID();
        }
        drawDropZone(nullptr, nullptr);
        ImGui::PopStyleVar(3);
        ImGui::Dummy(ImGui::GetContentRegionAvail());

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_editor->setSelectedWidget(nullptr);
        }

        if (!m_isWidgetHovered && ImGui::BeginPopupContextWindow()) {
            openCreateWidgetMenu("Create", nullptr);
            ImGui::EndPopup();
        }

        m_editor->getWidgetsRegistry().applyCommands();
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

    void UIHierarchyPanel::drawDropZone(const std::shared_ptr<Widget>& parent, const std::shared_ptr<Widget>& targetBefore) {
        float dropZoneHeight = 5; // Height of the invisible gap
        const ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
        ImRect dropRect(
            cursorScreenPos.x,
            cursorScreenPos.y, // Start at the top of the gap
            cursorScreenPos.x + ImGui::GetContentRegionAvail().x,
            cursorScreenPos.y + dropZoneHeight // End at the bottom of the gap (our current cursor)
        );

        const float lineY = dropRect.Min.y + dropZoneHeight / 2;
        const ImGuiID dropTargetId = ImGui::GetID((void*)((char*)targetBefore.get() + 1));

        if (ImGui::BeginDragDropTargetCustom(dropRect, dropTargetId)) {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WIDGET_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect | ImGuiDragDropFlags_AcceptPeekOnly);
            if (payload) {
                ImU32 feedbackColor = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
                ImGui::GetWindowDrawList()->AddLine({dropRect.Min.x, lineY}, {dropRect.Max.x, lineY}, feedbackColor, 2.5f);

                if (ImGui::AcceptDragDropPayload("WIDGET_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                    auto draggedWidget = *static_cast<std::shared_ptr<Widget>*>(payload->Data);

                    int newSiblingIndex = 0;
                    if (parent) {
                        auto& siblings = parent->m_children;
                        if (targetBefore) {
                            auto it = std::find(siblings.begin(), siblings.end(), targetBefore);
                            newSiblingIndex = std::distance(siblings.begin(), it);
                        } else {
                            newSiblingIndex = siblings.size();
                        }
                    } else {
                        const auto& roots = m_editor->getWidgetsRegistry().getRootWidgets();
                        if (targetBefore) {
                            auto it = std::find(roots.begin(), roots.end(), targetBefore);
                            newSiblingIndex = std::distance(roots.begin(), it);
                        } else {
                            newSiblingIndex = roots.size();
                        }
                    }

                    auto command = std::make_unique<MoveWidgetCommand>(
                        m_editor->getWidgetsRegistry(),
                        draggedWidget,
                        parent,
                        newSiblingIndex
                    );
                    m_editor->getWidgetsRegistry().queueCommand(std::move(command));
                    setHierarchyDirty();
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Advance the cursor past our invisible gap
        ImGui::Dummy({0, dropZoneHeight});
    }


    void UIHierarchyPanel::drawWidgetNode(HierarchyNode& node) {
        if (!node.widget) return;

        ImGui::PushID(node.widget.get());

        ImGui::Indent((node.depth + 1) * ImGui::GetStyle().IndentSpacing);

        bool isSelected = m_editor->getSelectedWidget() == node.widget;
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (isSelected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (node.widget->m_children.empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        bool opened = ImGui::TreeNodeEx("##TreeNode", flags, "%s", node.widget->getName().c_str());
        m_isWidgetHovered |= ImGui::IsItemHovered();

        if (ImGui::IsItemClicked(0) && !ImGui::IsItemToggledOpen()) {
            m_editor->setSelectedWidget(node.widget);
        }

        if (ImGui::IsItemToggledOpen()) {
            node.widget->m_isHierarchyOpen = opened;
            setHierarchyDirty();
        }

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("WIDGET_DRAG", &node.widget, sizeof(std::shared_ptr<Widget>));
            ImGui::Text("%s", node.widget->getName().c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            if (payload && payload->IsDataType("WIDGET_DRAG")) {
                auto draggedWidget = *static_cast<std::shared_ptr<Widget>*>(payload->Data);

                if (draggedWidget != node.widget && !node.widget->hasChild(draggedWidget)) {
                    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_DragDropTarget));

                    if (ImGui::AcceptDragDropPayload("WIDGET_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                        auto command = std::make_unique<MoveWidgetCommand>(
                            m_editor->getWidgetsRegistry(),
                            draggedWidget,
                            node.widget, // The new parent is this widget
                            node.widget->m_children.size() // Sibling index is the end of the list
                        );
                        m_editor->getWidgetsRegistry().queueCommand(std::move(command));
                        setHierarchyDirty();
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // --- Right-Click Context Menu ---
        if (ImGui::BeginPopupContextItem()) {
            m_editor->setSelectedWidget(node.widget); // Select the widget when right-clicking

            std::string nameBuffer = node.widget->getName();
            openCreateWidgetMenu("Create Child", node.widget);
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
                TE_LOGGER_WARN("UIHierarchyPanel: Delete action is not implemented yet.");
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

    void UIHierarchyPanel::openCreateWidgetMenu(const std::string& title, const std::shared_ptr<Widget>& parent) {
        if (ImGui::BeginMenu(title.c_str())) {
            ImGui::Text("BaseWidgets");
            if (ImGui::MenuItem("Container")) {
                m_editor->getWidgetsRegistry().createWidget(parent, "Container", "Container", false);
                setHierarchyDirty();
            } else if (ImGui::MenuItem("Panel")) {
                m_editor->getWidgetsRegistry().createWidget(parent, "Panel", "Panel", false);
                setHierarchyDirty();
            } else if (ImGui::MenuItem("Text")) {
                m_editor->getWidgetsRegistry().createWidget(parent, "Text", "Text", false);
                setHierarchyDirty();
            } else if (ImGui::MenuItem("InputText")) {
                m_editor->getWidgetsRegistry().createWidget(parent, "InputText", "InputText", false);
                setHierarchyDirty();
            } else if (ImGui::MenuItem("Interactable")) {
                m_editor->getWidgetsRegistry().createWidget(parent, "Interactable", "Interactable", false);
                setHierarchyDirty();
            } /*else if (ImGui::MenuItem("Image")) {
            m_editor->createWidget(nullptr, "Image", "Image");
        }*/
            ImGui::Separator();
            ImGui::Text("Widgets");
            for (const WidgetsRegistry::CustomWidgetTemplate& widget: m_editor->getWidgetsRegistry().getWidgetTemplates()) {
                if (ImGui::MenuItem(widget.name.c_str())) {
                    ImGui::Text("%s", widget.name.c_str());
                    m_editor->getWidgetsRegistry().createWidget(parent, widget.name, widget.type, true);
                    setHierarchyDirty();
                }
            }
            ImGui::EndMenu();
        }
    }
}
