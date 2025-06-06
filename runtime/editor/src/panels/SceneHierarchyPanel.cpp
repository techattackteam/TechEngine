#include "SceneHierarchyPanel.hpp"

#include "resources/ResourcesManager.hpp"
#include "scene/ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel(SystemsRegistry& editorSystemRegistry,
                                             SystemsRegistry& appSystemRegistry,
                                             std::vector<Entity>& selectedEntities) : m_appSystemRegistry(
            appSystemRegistry),
        m_selectedEntities(selectedEntities), Panel(editorSystemRegistry) {
    }


    void SceneHierarchyPanel::onInit() {
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Tag>([this](Tag& tag) {
            m_entitiesOrder.emplace_back(tag);
        });
    }

    void SceneHierarchyPanel::onUpdate() {
        isItemHovered = false;
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f)); // Adjust spacing between items
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 3.0f)); // Compact frame padding
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f); // Reduce indentation

        std::vector<Tag> entitiesOrderCopy = m_entitiesOrder;
        for (size_t i = 0; i < entitiesOrderCopy.size(); i++) {
            drawEntityNode(scene.getComponent<Tag>(scene.getEntityByTag(entitiesOrderCopy[i])));
        }

        ImGui::PopStyleVar(3); // Must match the number of PushStyleVar calls

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_selectedEntities.clear();
        }

        if (!isItemHovered && ImGui::BeginPopupContextWindow()) {
            openCreateMenu("Create", -1);
            ImGui::EndPopup();
        }
        if (!m_entitiesToDelete.empty()) {
            for (auto entity: m_entitiesToDelete) {
                // If the entity to delete was selected, deselect it
                std::erase(m_selectedEntities, entity);
                scene.destroyEntity(entity);
            }
            m_entitiesToDelete.clear();

            // After deleting, the entitiesOrder list is out of sync. Rebuild it.
            // This is the safest way to ensure correctness.
            m_entitiesOrder.clear();
            scene.runSystem<Tag>([this](Tag& tag) {
                m_entitiesOrder.emplace_back(tag);
            });
        }
        if (!m_entitiesToCreate.empty()) {
            requestEntitiesCreation();
        }
    }

    void SceneHierarchyPanel::drawEntityNode(Tag& tag) {
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
        Entity entity = scene.getEntityByTag(tag);
        bool isSelected = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity) != m_selectedEntities.
                          end();


        // Draw the entity node
        ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_FramePadding |
                                   ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx((void*)(intptr_t)entity, flags, "%s", tag.getName().c_str());
        isItemHovered |= ImGui::IsItemHovered();

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

        if (ImGui::BeginDragDropTarget()) {
            isItemHovered = true; // Dropping onto an item counts as hovering

            // Get the position and size of the tree node item
            const ImVec2 targetMin = ImGui::GetItemRectMin();
            const ImVec2 targetMax = ImGui::GetItemRectMax();
            const ImVec2 mousePos = ImGui::GetMousePos();
            const float targetHeight = targetMax.y - targetMin.y;

            // Define a threshold for the top/bottom reorder zones (e.g., 25% of the item height)
            const float reorderThreshold = 0.275f;

            // Determine the drop action based on mouse position
            enum class DropAction { None, Reparent, ReorderAbove, ReorderBelow };
            DropAction dropAction = DropAction::None;

            if (mousePos.y < targetMin.y + targetHeight * reorderThreshold) {
                dropAction = DropAction::ReorderAbove;
            } else if (mousePos.y > targetMax.y - targetHeight * reorderThreshold) {
                dropAction = DropAction::ReorderBelow;
            } else {
                dropAction = DropAction::Reparent;
            }

            // Provide visual feedback based on the determined action
            ImU32 feedbackColor = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
            switch (dropAction) {
                case DropAction::ReorderAbove:
                    ImGui::GetWindowDrawList()->AddLine(targetMin, ImVec2(targetMax.x, targetMin.y), feedbackColor,
                                                        2.0f);
                    break;
                case DropAction::ReorderBelow:
                    ImGui::GetWindowDrawList()->AddLine(ImVec2(targetMin.x, targetMax.y), targetMax, feedbackColor,
                                                        2.0f);
                    break;
                case DropAction::Reparent:
                    ImGui::GetWindowDrawList()->AddRect(targetMin, targetMax, feedbackColor, 2.0f, 0, 2.0f);
                    break;
                default: break;
            }

            // Handle the actual drop
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                "ENTITY_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                Entity draggedEntity = *(Entity*)payload->Data;
                if (draggedEntity != entity) {
                    int index = std::ranges::find(m_entitiesOrder, tag) - m_entitiesOrder.begin();
                    switch (dropAction) {
                        case DropAction::ReorderAbove:
                            reorderEntity(scene.getComponent<Tag>(draggedEntity), index);
                            break;
                        case DropAction::ReorderBelow:
                            reorderEntity(scene.getComponent<Tag>(draggedEntity), index + 1);
                            break;
                        case DropAction::Reparent:
                            TE_LOGGER_INFO("Parenting entity {} under entity {}", draggedEntity, entity);
                            break;
                        default: break;
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }


        // Context menu for additional actions
        if (ImGui::BeginPopupContextItem()) {
            m_selectedEntities.clear();
            m_selectedEntities.push_back(entity);

            std::string name = tag.getName();
            openCreateMenu("Create Child (WIP)", scene.getEntityByTag(tag));
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "Name", name)) {
                tag.setName(name);
            }
            if (ImGui::MenuItem("Delete Entity")) {
                m_entitiesToDelete.push_back(entity);
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
    }

    void SceneHierarchyPanel::reorderEntity(Tag& tag, size_t newPosition) {
        auto it = std::find(m_entitiesOrder.begin(), m_entitiesOrder.end(), tag);
        if (it != m_entitiesOrder.end()) {
            // Adjust newPosition if the item is moved downwards
            if (std::distance(m_entitiesOrder.begin(), it) < newPosition) {
                newPosition--;
            }
            m_entitiesOrder.erase(it);
            m_entitiesOrder.insert(m_entitiesOrder.begin() + newPosition, tag);
            TE_LOGGER_INFO("Reordered entity {} to position {}", tag.getName(), newPosition);
        }
    }

    void SceneHierarchyPanel::requestEntitiesCreation() {
        for (EntityToCreate& entityType: m_entitiesToCreate) {
            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
            switch (entityType.type) {
                case Empty: {
                    Entity entity = scene.createEntity("New Entity");
                    m_entitiesOrder.emplace_back(scene.getComponent<Tag>(entity));
                    break;
                }
                case Cube: {
                    Entity entity = scene.createEntity("Cube");
                    ResourcesManager& resourcesManager = m_appSystemRegistry.getSystem<ResourcesManager>();
                    scene.addComponent<MeshRenderer>(
                        entity, MeshRenderer(resourcesManager.getDefaultMesh(), resourcesManager.getDefaultMaterial()));
                    m_entitiesOrder.emplace_back(scene.getComponent<Tag>(entity));
                    break;
                }
                default:
                    break;
            }
        }
        m_entitiesToCreate.clear();
    }

    void SceneHierarchyPanel::openCreateMenu(std::string title, Entity parent = -1) {
        if (ImGui::BeginMenu(title.c_str())) {
            if (ImGui::MenuItem("Empty")) {
                m_entitiesToCreate.push_back({"New Entity", Empty, parent});
                //Entity entity = scene.createEntity("New Entity");
                //entitiesOrder.emplace_back(scene.getComponent<Tag>(entity));
            }
            if (ImGui::MenuItem("Cube")) {
                m_entitiesToCreate.push_back({"Cube", Cube, parent});
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
    }
}
