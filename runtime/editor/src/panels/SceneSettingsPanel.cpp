#include "SceneSettingsPanel.hpp"

#include <imgui_internal.h>

#include "UIEditor.hpp"
#include "profiling/ProfiledScope.hpp"
#include "project/ProjectManager.hpp"
#include "resources/ResourcesManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "scene/ScenesManager.hpp"
#include "TechEngine/core/components/ComponentsFactory.hpp"
#include "ui/Widget.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    SceneSettingsPanel::SceneSettingsPanel(ProjectType& projectType,
                                           SystemsRegistry& editorSystemsRegistry,
                                           SystemsRegistry& appSystemsRegistry,
                                           HierarchyNode& selectedNode) : Panel(editorSystemsRegistry),
                                                                          m_projectType(projectType),
                                                                          m_appSystemsRegistry(appSystemsRegistry),
                                                                          m_selectedNode(selectedNode) {
    }


    void SceneSettingsPanel::onInit() {
        HierarchyNode node;
        node.id = 1;
        node.name = "Ambient Occlusion Settings";
        node.type = HierarchyNode::NodeType::RenderPass;
        node.depth = 0;
        node.isOpen = true;
        node.renderPassName = new std::string("Ambient Occlusion Settings");
        m_displayRenderPassList.push_back(node);
        node.id = 2;
        node.name = "Bloom Settings";
        node.renderPassName = new std::string("Bloom Settings");
        m_displayRenderPassList.push_back(node);
        node.id = 3;
        node.name = "Chromatic Aberration Settings";
        node.renderPassName = new std::string("Chromatic Aberration Settings");
        m_displayRenderPassList.push_back(node);
        node.id = 4;
        node.name = "Vignette Settings";
        node.renderPassName = new std::string("Vignette Settings");
        m_displayRenderPassList.push_back(node);
        node.id = 5;
        node.name = "Gamma Settings";
        node.renderPassName = new std::string("Gamma Settings");
        m_displayRenderPassList.push_back(node);
        node.id = 6;
        node.name = "Color Grading Settings";
        node.renderPassName = new std::string("Color Grading Settings");
        m_displayRenderPassList.push_back(node);
        node.id = 7;
        node.name = "Film Grain Settings";
        node.renderPassName = new std::string("Film Grain Settings");
        m_displayRenderPassList.push_back(node);
        node.id = 8;
        node.name = "Fog Settings";
        node.renderPassName = new std::string("Fog Settings");
        m_displayRenderPassList.push_back(node);
        node.id = 9;
        node.name = "God Rays Settings";
        node.renderPassName = new std::string("God Rays Settings");
        m_displayRenderPassList.push_back(node);
    }

    void SceneSettingsPanel::onUpdate() {
        m_isNodeHovered = false;
        // Add a bit of vertical spacing between our main styled nodes
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 8.0f));

        // Use the new helper for the "Entities" tree
        if ((m_projectType == ProjectType::Client || m_projectType == ProjectType::Server) && drawStyledMainTreeNode("Entities")) {
            drawTree(HierarchyNode::NodeType::Entity); // Renamed from your original code for clarity
            ImGui::TreePop();
        }

        // Use the new helper for the "UI" tree
        if (m_projectType == ProjectType::Client && drawStyledMainTreeNode("UI")) {
            drawTree(HierarchyNode::NodeType::Widget); // Renamed from your original code for clarity
            ImGui::TreePop();
        }

        // Use the new helper for the "Renderer" tree
        if (m_projectType == ProjectType::Client && drawStyledMainTreeNode("Renderer")) {
            drawTree(HierarchyNode::NodeType::RenderPass); // Renamed from your original code for clarity
            ImGui::TreePop();
        }

        ImGui::PopStyleVar(); // Pop the ItemSpacing override

        // Deselect if clicking in an empty area of the panel
        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_selectedNode = HierarchyNode();
        }

        if (!m_isNodeHovered && ImGui::BeginPopupContextWindow()) {
            openRightClickMenu(HierarchyNode::NodeType::Entity);
            ImGui::EndPopup();
        }
        if (!m_nodesToDelete.empty()) {
            Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
            m_selectedNode = HierarchyNode();
            for (auto node: m_nodesToDelete) {
                scene.destroyEntity(node.entity);
            }
            m_nodesToDelete.clear();

            m_isHierarchyDirty = true;
        }
        if (!m_nodesToCreate.empty()) {
            requestEntitiesCreation();
        }
    }

    void SceneSettingsPanel::drawTree(HierarchyNode::NodeType nodeType) {
        std::vector<HierarchyNode>* displayList = nullptr;
        if (nodeType == HierarchyNode::NodeType::Entity) {
            displayList = &m_displayEntitiesList;
        } else if (nodeType == HierarchyNode::NodeType::Widget) {
            displayList = &m_displayWidgetList;
        } else if (nodeType == HierarchyNode::NodeType::RenderPass) {
            displayList = &m_displayRenderPassList;
        } else {
            TE_LOGGER_CRITICAL("SceneSettingsPanel::drawTree: Unsupported node type.");
            return;
        }

        if (m_isHierarchyDirty) {
            rebuildDisplayList(nodeType);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 1.0f)); // Adjust spacing between items
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f)); // Compact frame padding
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f); // Reduce indentation
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(displayList->size())); {
            ProfiledScope profiler(m_editorSystemsRegistry.getSystem<Profiler>(), "SceneHierarchyPanel::drawEntityNode loop");
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    if (i < 0 || i >= displayList->size()) {
                        continue;
                    }
                    HierarchyNode& node = displayList->at(i);
                    ImGui::PushID(node.id);
                    drawDropZone(node.widget->m_parent, node.widget);
                    drawNode(node);
                    ImGui::PopID();
                }
            }
        }
        drawDropZone(nullptr, nullptr);
        ImGui::PopStyleVar(3);

        /*if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_selectedNode = HierarchyNode();
        }*/


        //m_editor->getWidgetsRegistry().applyCommands();
    }

    bool SceneSettingsPanel::drawStyledMainTreeNode(const char* label) {
        const ImGuiTreeNodeFlags treeNodeFlags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_Framed; // Use Framed flag for a background

        // --- Style Overrides ---
        // A nice blue color for the background
        ImVec4 blueColor = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        // A slightly lighter blue for when the header is hovered
        ImVec4 blueColorHovered = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        // An even lighter blue for when it's active/clicked
        ImVec4 blueColorActive = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

        // Push the colors for the "Header" element, which TreeNodeEx uses
        ImGui::PushStyleColor(ImGuiCol_Header, blueColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, blueColorHovered);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, blueColorActive);

        // Push the rounded corners style
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        // Add a bit of vertical padding to make it look better
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 2.0f));

        bool open = ImGui::TreeNodeEx(label, treeNodeFlags);

        // We pushed 2 style vars and 3 style colors.
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        return open;
    }

    void SceneSettingsPanel::drawRendererTree() {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (ImGui::TreeNodeEx("Renderer", treeNodeFlags)) {
            const char* passes[] = {"Shadows", "Ambient Occlusion", "Bloom", "Post-Processing"}; // Simplified list

            for (const char* passName: passes) {
                if (ImGui::Selectable(passName)) {
                    TE_LOGGER_INFO("Render Pass '{}' selected in Scene Settings Panel", passName);
                }
            }

            ImGui::TreePop();
        }
    }

    void SceneSettingsPanel::drawNode(HierarchyNode& node) {
        ImGui::PushID(node.id);

        ImGui::Indent((node.depth + 1) * ImGui::GetStyle().IndentSpacing);

        bool isSelected = m_selectedNode == node;
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (isSelected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (true /*node->m_children.empty()*/) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        ImGui::PushID((void*)(intptr_t)node.id);
        bool opened = ImGui::TreeNodeEx("##TreeNode", flags, "%s", node.name.c_str());
        m_isNodeHovered |= ImGui::IsItemHovered();
        ImGui::PopID();

        if (ImGui::IsItemClicked(0) && !ImGui::IsItemToggledOpen()) {
            m_selectedNode = node;
        }
        if (ImGui::IsItemToggledOpen()) {
            node.isOpen = opened;
            setHierarchyDirty();
        }

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("NODE_DRAG", &node, sizeof(HierarchyNode));
            ImGui::Text("%s", node.name.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget()) {
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            if (payload && payload->IsDataType("NODE_DRAG")) {
                /*
                auto draggedWidget = *static_cast<std::shared_ptr<Widget>*>(payload->Data);

                if (draggedWidget != node.widget && !node.widget->hasChild(draggedWidget)) {
                    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImGui::GetColorU32(ImGuiCol_DragDropTarget));

                    if (ImGui::AcceptDragDropPayload("NODE_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
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
                */
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem("NodeRightClickMenu")) {
            m_selectedNode = node;
            openRightClickMenu(node);
            /*
            //m_editor->setSelectedWidget(node.widget); // Select the widget when right-clicking

            std::string nameBuffer = node.name;
            //openCreateWidgetMenu("Create Child", node.widget);
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "##Rename", nameBuffer)) {
                /*node.widget->rename(nameBuffer);
                ImGui::CloseCurrentPopup();#1#
            }

            if (ImGui::MenuItem("Delete")) {
                TE_LOGGER_WARN("UIHierarchyPanel: Delete action is not implemented yet.");
            }
            ImGui::EndPopup();*/
        }


        if (opened) {
            ImGui::TreePop();
        }

        ImGui::Unindent((node.depth + 1) * ImGui::GetStyle().IndentSpacing);

        ImGui::PopID();
    }

    void SceneSettingsPanel::drawDropZone(const std::shared_ptr<Widget>& parent, const std::shared_ptr<Widget>& targetBefore) {
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

                /*if (ImGui::AcceptDragDropPayload("WIDGET_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
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
                }*/
            }
            ImGui::EndDragDropTarget();
        }

        // Advance the cursor past our invisible gap
        ImGui::Dummy({0, dropZoneHeight});
    }

    void SceneSettingsPanel::rebuildDisplayList(HierarchyNode::NodeType nodeType) {
        m_displayWidgetList.clear();
        if (nodeType == HierarchyNode::NodeType::Entity) {
            m_displayEntitiesList.clear();
            Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
            scene.runSystem<Entity, Tag>([this](Entity entity, Tag& tag) {
                HierarchyNode node;
                node.name = tag.getName();
                node.type = HierarchyNode::NodeType::Entity;
                node.entity = entity;
                node.id = entity;
                node.depth = 0; // Flat list for now
                node.isOpen = false;
                m_displayEntitiesList.push_back(node);
            });
            return;
        }
        for (const std::shared_ptr<Widget>& widget: m_appSystemsRegistry.getSystem<WidgetsRegistry>().getWidgets()) {
            if (widget && widget->m_parent == nullptr) {
                recursiveAddToDisplayList(widget, 0);
            }
        }
        m_isHierarchyDirty = false;
    }

    void SceneSettingsPanel::recursiveAddToDisplayList(const std::shared_ptr<Widget>& widget, int depth) {
        HierarchyNode node;
        node.type = HierarchyNode::NodeType::Widget;
        node.widget = widget;
        node.id = reinterpret_cast<uintptr_t>(widget.get());
        node.depth = depth;
        node.isOpen = false;
        m_displayWidgetList.push_back(node);

        for (const auto& child: widget->m_children) {
            recursiveAddToDisplayList(child, depth + 1);
        }
    }

    void SceneSettingsPanel::openRightClickMenu(HierarchyNode& clickedNode) {
        if (clickedNode.type == HierarchyNode::NodeType::Entity) {
            openEntityMenu(clickedNode);
        }
    }

    void SceneSettingsPanel::openEntityMenu(HierarchyNode& clickedNode) {
        Entity entity = clickedNode.entity;
        Tag& tag = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene().getComponent<Tag>(entity);
        std::string name = clickedNode.name;
        openCreateEntityMenu("Create Child (WIP)", clickedNode.entity);
        if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "Name", name)) {
            tag.setName(name);
        }
        if (ImGui::MenuItem("Delete Entity")) {
            m_nodesToDelete.push_back(clickedNode);
        }
        ImGui::EndPopup();
    }

    void SceneSettingsPanel::openRightClickMenu(HierarchyNode::NodeType nodeType) {
        if (nodeType == HierarchyNode::NodeType::Entity) {
            openCreateEntityMenu("Create", -1);
        }
    }

    void SceneSettingsPanel::openCreateEntityMenu(std::string title, Entity parent) {
        if (ImGui::BeginMenu(title.c_str())) {
            if (ImGui::MenuItem("Empty")) {
                m_nodesToCreate.push_back({"New Entity", Empty});
            }

            if (ImGui::MenuItem("Cube")) {
                m_nodesToCreate.push_back({"Cube", Cube, parent});
            }

            if (ImGui::MenuItem("Sphere")) {
                m_nodesToCreate.push_back({"Sphere", Sphere, parent});
            }

            if (ImGui::MenuItem("Cylinder")) {
                //m_entitiesToCreate.push_back({"Cylinder", Cylinder, parent});
            }
            ImGui::EndMenu();
        }
    }

    void SceneSettingsPanel::requestEntitiesCreation() {
        for (EntityToCreate& entityType: m_nodesToCreate) {
            Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
            switch (entityType.type) {
                case Empty: {
                    scene.createEntity("New Entity");
                }
                case Cube: {
                    Entity entity = scene.createEntity("Cube");
                    ResourcesManager& resourcesManager = m_appSystemsRegistry.getSystem<ResourcesManager>();
                    scene.addComponent<MeshRenderer>(entity, ComponentsFactory::createMeshRenderer(&resourcesManager.getDefaultMesh(), &resourcesManager.getDefaultMaterial()));
                    break;
                }
                case Sphere: {
                    Entity entity = scene.createEntity("Sphere");
                    ResourcesManager& resourcesManager = m_appSystemsRegistry.getSystem<ResourcesManager>();
                    scene.addComponent<MeshRenderer>(entity, ComponentsFactory::createMeshRenderer(&resourcesManager.getMesh("Sphere"), &resourcesManager.getDefaultMaterial()));
                    break;
                }
                default:
                    break;
            }
        }
        m_nodesToCreate.clear();
        m_isHierarchyDirty = true;
    }

    void SceneSettingsPanel::setHierarchyDirty() {
    }
}
