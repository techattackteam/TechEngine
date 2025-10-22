#include "SceneHierarchyPanel.hpp"

#include "imgui_internal.h"
#include "TechEngine/core/components/ComponentsFactory.hpp"
#include "profiling/ProfiledScope.hpp"
#include "profiling/Profiler.hpp"
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
        scene.runSystem<Entity, Tag>([this](Entity& entity, Tag& tag) {
            if (std::find(m_entitiesOrder.begin(), m_entitiesOrder.end(), entity) == m_entitiesOrder.end()) {
                m_entitiesOrder.emplace_back(entity);
            }
        });
    }

    void SceneHierarchyPanel::onUpdate() {
        m_isItemHovered = false;
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene(); {
            ProfiledScope profiler(m_editorSystemsRegistry.getSystem<Profiler>(), "SceneHierarchyPanel::drawEntityNode gather");
            scene.runSystem<Entity, Tag>([this](Entity& entity, Tag& tag) {
                if (std::find(m_entitiesOrder.begin(), m_entitiesOrder.end(), entity) == m_entitiesOrder.end()) {
                    m_entitiesOrder.emplace_back(entity);
                }
            });
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f)); // Adjust spacing between items
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f)); // Compact frame padding
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f); // Reduce indentation
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_entitiesOrder.size())); {
            ProfiledScope profiler(m_editorSystemsRegistry.getSystem<Profiler>(), "SceneHierarchyPanel::drawEntityNode loop");
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    if (i < 0 || i >= m_entitiesOrder.size()) {
                        continue;
                    }

                    Entity entity = m_entitiesOrder[i];
                    if (entity == -1) {
                        continue;
                    }
                    drawEntityNode(entity, i);
                }
            }
        }
        ImGui::PopStyleVar(3);

        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_selectedEntities.clear();
        }

        if (!m_isItemHovered && ImGui::BeginPopupContextWindow()) {
            openCreateMenu("Create", -1);
            ImGui::EndPopup();
        }
        if (!m_entitiesToDelete.empty()) {
            for (auto entity: m_entitiesToDelete) {
                std::erase(m_selectedEntities, entity);
                scene.destroyEntity(entity);
            }
            m_entitiesToDelete.clear();

            m_entitiesOrder.clear();
            scene.runSystem<Entity, Tag>([this](Entity entity, Tag& tag) {
                m_entitiesOrder.emplace_back(entity);
            });
        }
        if (!m_entitiesToCreate.empty()) {
            requestEntitiesCreation();
        }
    }

    void SceneHierarchyPanel::drawEntityNode(Entity& entity, int i) {
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
        ImGuiStyle& style = ImGui::GetStyle();
        Tag& tag = scene.getComponent<Tag>(entity);

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

            if (payload && payload->DataSize == sizeof(Entity)) {
                Entity draggedEntity = *(Entity*)payload->Data;

                bool isSameEntity = false;
                if (draggedEntity == entity) {
                    isSameEntity = true;
                }

                if (!reparentRect.Contains(mousePos)) {
                    dropAction = DropAction::Reorder;
                }

                if (!isSameEntity && dropAction == DropAction::Reorder) {
                    auto originalEntity = std::find(m_entitiesOrder.begin(), m_entitiesOrder.end(), draggedEntity);
                    auto targetEntity = std::find(m_entitiesOrder.begin(), m_entitiesOrder.end(), entity);
                    if (originalEntity != m_entitiesOrder.end() && targetEntity != m_entitiesOrder.end()) {
                        bool isDroppingAbove = (mousePos.y < reparentRect.Min.y);
                        if (isDroppingAbove) {
                            if (std::next(originalEntity) == targetEntity) {
                                isSameEntity = true;
                            }
                        } else {
                            if (targetEntity != m_entitiesOrder.begin() && std::next(targetEntity) == originalEntity) {
                                isSameEntity = true;
                            }
                        }
                    }
                }

                if (!isSameEntity) {
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
                "ENTITY_DRAG", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                int index = *(Entity*)payload->Data;
                if (index != entity) {
                    if (dropAction == DropAction::Reorder) {
                        int newIndex = (mousePos.y < reparentRect.Min.y) ? index : index + 1;
                        reorderEntity(entity, newIndex);
                    } else {
                        TE_LOGGER_INFO("Parenting entity {} under entity {}", index, entity);
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

        bool isSelected = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity) != m_selectedEntities.
                          end();
        ImGuiTreeNodeFlags flags = (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanAvailWidth |
                                   ImGuiTreeNodeFlags_Leaf;

        ImGui::PushID((void*)(intptr_t)entity);
        bool opened = ImGui::TreeNodeEx("##TreeNode", flags, "%s", tag.getName().c_str());
        m_isItemHovered |= ImGui::IsItemHovered();

        ImGui::PopID();

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("ENTITY_DRAG", &i, sizeof(int));
            ImGui::Text("%s", tag.getName());
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemClicked()) {
            m_selectedEntities.clear();
            m_selectedEntities.push_back(entity);
        }

        if (ImGui::BeginPopupContextItem()) {
            m_selectedEntities.clear();
            m_selectedEntities.push_back(entity);

            std::string name = tag.getName();
            openCreateMenu("Create Child (WIP)", entity);
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "Name", name)) {
                tag.setName(name);
            }
            if (ImGui::MenuItem("Delete Entity")) {
                m_entitiesToDelete.push_back(entity);
            }
            ImGui::EndPopup();
        }

        ImGui::SetCursorPosY(initialCursorPos.y + totalItemHeight);

        if (opened) {
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::reorderEntity(Entity& entity, size_t newPosition) {
        auto it = std::find(m_entitiesOrder.begin(), m_entitiesOrder.end(), entity);
        if (it != m_entitiesOrder.end()) {
            // Adjust newPosition if the item is moved downwards
            if (std::distance(m_entitiesOrder.begin(), it) < newPosition) {
                newPosition--;
            }
            m_entitiesOrder.erase(it);
            m_entitiesOrder.insert(m_entitiesOrder.begin() + newPosition, entity);
        }
    }

    void SceneHierarchyPanel::requestEntitiesCreation() {
        for (EntityToCreate& entityType: m_entitiesToCreate) {
            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
            switch (entityType.type) {
                case Empty: {
                    Entity entity = scene.createEntity("New Entity");
                    m_entitiesOrder.emplace_back(entity);
                    break;
                }
                case Cube: {
                    Entity entity = scene.createEntity("Cube");
                    ResourcesManager& resourcesManager = m_appSystemRegistry.getSystem<ResourcesManager>();
                    scene.addComponent<MeshRenderer>(entity, ComponentsFactory::createMeshRenderer(&resourcesManager.getDefaultMesh(), &resourcesManager.getDefaultMaterial()));
                    m_entitiesOrder.emplace_back(entity);
                    break;
                }
                case Sphere: {
                    Entity entity = scene.createEntity("Sphere");
                    ResourcesManager& resourcesManager = m_appSystemRegistry.getSystem<ResourcesManager>();
                    scene.addComponent<MeshRenderer>(entity, ComponentsFactory::createMeshRenderer(&resourcesManager.getMesh("Sphere"), &resourcesManager.getDefaultMaterial()));
                    m_entitiesOrder.emplace_back(entity);
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
            }

            if (ImGui::MenuItem("Cube")) {
                m_entitiesToCreate.push_back({"Cube", Cube, parent});
            }

            if (ImGui::MenuItem("Sphere")) {
                m_entitiesToCreate.push_back({"Sphere", Sphere, parent});
            }

            if (ImGui::MenuItem("Cylinder")) {
                //m_entitiesToCreate.push_back({"Cylinder", Cylinder, parent});
            }
            ImGui::EndMenu();
        }
    }
}
