#include "SceneHierarchyPanel.hpp"

#include "resources/ResourcesManager.hpp"
#include "scene/ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel(SystemsRegistry& editorSystemRegistry,
                                             SystemsRegistry& appSystemRegistry,
                                             std::vector<Entity>& selectedEntities) : m_appSystemRegistry(appSystemRegistry),
                                                                                      m_selectedEntities(selectedEntities), Panel(editorSystemRegistry) {
    }


    void SceneHierarchyPanel::onInit() {
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Tag>([this](Tag& tag) {
            entitiesOrder.emplace_back(tag);
        });
    }

    void SceneHierarchyPanel::onUpdate() {
        isItemHovered = false;
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f)); // Adjust spacing between items
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f)); // Compact frame padding
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f); // Reduce indentation

        drawDropZone(0);
        for (const auto& tag: entitiesOrder) {
            drawEntityNode(scene.getComponent<Tag>(scene.getEntityByTag(tag)));
        }
        drawDropZone(scene.getTotalEntities());

        ImGui::PopStyleVar(3); // Must match the number of PushStyleVar calls

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_selectedEntities.clear();
        }

        if (!isItemHovered && ImGui::BeginPopupContextWindow()) {
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Empty")) {
                    Entity entity = scene.createEntity("New Entity");
                    entitiesOrder.emplace_back(scene.getComponent<Tag>(entity));
                }
                if (ImGui::MenuItem("Cube")) {
                    Entity entity = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene().createEntity("Cube");
                    ResourcesManager& resourcesManager = m_appSystemRegistry.getSystem<ResourcesManager>();
                    m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene().addComponent<MeshRenderer>(entity, MeshRenderer(resourcesManager.getDefaultMesh(), resourcesManager.getDefaultMaterial()));
                }
                if (ImGui::MenuItem("Sphere")) {
                    /*GameObject& gameObject = appRegistry.getSystem<SceneManager>().getScene().createGameObject("Sphere");
                    gameObject.addComponent<MeshRendererComponent>();*/
                }
                if (ImGui::MenuItem("Cylinder")) {
                    /*GameObject& gameObject = appRegistry.getSystem<SceneManager>().getScene().createGameObject("Cylinder");
                    gameObject.addComponent<MeshRendererComponent>();*/
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }
    }

    void SceneHierarchyPanel::drawEntityNode(Tag& tag) {
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
        Entity entity = scene.getEntityByTag(tag);
        bool isSelected = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity) != m_selectedEntities.end();


        // Draw the entity node
        ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, "%s", tag.getName().c_str());

        // Selection handling
        if (ImGui::IsItemClicked()) {
            m_selectedEntities.clear();
            m_selectedEntities.push_back(entity);
        }

        // Drag-and-Drop Source
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("ENTITY_DRAG", &entity, sizeof(Entity)); // Set payload
            ImGui::Text("%s", tag.getName().c_str()); // Display dragged entity name
            ImGui::EndDragDropSource();
        }

        // Drag-and-Drop Target (for making this a parent)
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG")) {
                Entity draggedEntity = *(Entity*)payload->Data; // Retrieve dragged entity

                if (draggedEntity != entity) {
                    // Make draggedEntity a child of the current entity
                    //scene.makeChild(entity, draggedEntity); // Replace with your hierarchy management logic
                    TE_LOGGER_INFO("Dropped entity {} onto entity {}", draggedEntity, entity);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Context menu for additional actions
        if (ImGui::BeginPopupContextItem()) {
            m_selectedEntities.clear();
            m_selectedEntities.push_back(entity);

            std::string name = tag.getName();
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "Name", name)) {
                // Rename entity logic here
            }
            if (ImGui::MenuItem("Delete Entity")) {
                scene.destroyEntity(entity);
            }
            ImGui::EndPopup();
        }

        // Draw child nodes if the node is opened
        if (opened) {
            /*for (const auto& child: scene.getChildren(entity)) { // Replace with your child retrieval logic
                drawEntityNode(scene.getComponent<Tag>(child));
            }
            */
            ImGui::TreePop();
        }

        int index = std::ranges::find(entitiesOrder, tag) - entitiesOrder.begin();
        // Draw a drop zone before the entity (for reordering above it)
        drawDropZone(index);
    }

    void SceneHierarchyPanel::reorderEntity(Tag& tag, size_t newPosition) {
        auto it = std::find(entitiesOrder.begin(), entitiesOrder.end(), tag);
        if (it != entitiesOrder.end()) {
            entitiesOrder.erase(it); // Remove the entity from its current position
        }
        entitiesOrder.insert(entitiesOrder.begin() + newPosition, tag); // Insert it at the new position
        TE_LOGGER_INFO("Reordered entity {} to position {}", tag.getName(), newPosition);
    }

    void SceneHierarchyPanel::drawDropZone(size_t position) {
        // Use an invisible button as a drop zone
        ImGui::PushID(position);

        ImGui::InvisibleButton("DropZone", ImVec2(ImGui::GetContentRegionAvail().y, 2.0f));
        // Highlight the drop zone if it's active
        if (ImGui::IsItemHovered() && ImGui::IsMouseDragging(0)) {
            ImGui::GetWindowDrawList()->AddRect(
                ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                IM_COL32(255, 255, 0, 255)); // Yellow highlight
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG")) {
                Entity draggedEntity = *(Entity*)payload->Data;
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                reorderEntity(scene.getComponent<Tag>(draggedEntity), position);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::PopID();
    }
}
