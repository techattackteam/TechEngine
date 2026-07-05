#include "EntityHierarchyPanel.hpp"

#include <algorithm>
#include <functional>
#include <unordered_set>

#include <glm/gtx/matrix_decompose.hpp>

#include "resources/ResourceSystem.hpp"
#include "scene/SceneManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/components/ComponentsFactory.hpp"
#include "TechEngine/core/events/scene/EntityCreatedEvent.hpp"
#include "TechEngine/core/events/scene/SceneLoadEvent.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    EntityHierarchyPanel::EntityHierarchyPanel(
        SystemsRegistry& editorSystemsRegistry,
        SystemsRegistry& appSystemsRegistry,
        HierarchyNode& selectedNode
    ) : HierarchyPanelBase<Entity>(editorSystemsRegistry, HierarchyPanelConfig{
                                       .allowReparenting = true,
                                       .allowReordering = true,
                                       .allowCreation = true,
                                       .allowDeletion = true,
                                       .allowRenaming = true,
                                       .showDropZones = true,
                                       .preserveWorldTransform = true
                                   }),
        m_appSystemsRegistry(appSystemsRegistry),
        m_legacySelectedNode(selectedNode) {
        m_dragDropPayloadName = "ENTITY_DRAG";
        m_nodeCache.reserve(256);
        m_rootNodes.reserve(64);
    }

    Scene& EntityHierarchyPanel::getActiveScene() const {
        return m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
    }

    void EntityHierarchyPanel::onHierarchyInit() {
        m_appSystemsRegistry.getSystem<EventManager>().subscribe<SceneLoadEvent>(
            [this](const std::shared_ptr<Event>& event) {
                markFullRebuildNeeded();
            }
        );
        m_appSystemsRegistry.getSystem<EventManager>().subscribe<EntityCreatedEvent>(
            [this](const std::shared_ptr<Event>& event) {
                markFullRebuildNeeded();
            }
        );
    }

    void EntityHierarchyPanel::rebuildCache() {
        std::unordered_map<Entity, bool> oldExpansionStates;
        for (const auto& [entity, cache]: m_nodeCache) {
            oldExpansionStates[entity] = cache.isExpanded;
        }

        // Save old root order to preserve user's manual ordering
        std::vector<Entity> oldRootOrder = m_rootNodes;

        m_nodeCache.clear();
        m_rootNodes.clear();
        m_dirtyNodes.clear();

        Scene& scene = getActiveScene();

        // Collect all current root entities
        std::unordered_set<Entity> currentRoots;

        m_totalNodeCount = 0;
        scene.runSystem<Entity, Tag>([this, &scene, &oldExpansionStates, &currentRoots](Entity entity, const Tag& tag) {
            ++m_totalNodeCount;

            NodeCache cache;
            cache.id = entity;
            cache.name = tag.getName();
            cache.isLoaded = true;

            if (scene.hasComponent<Hierarchy>(entity)) {
                const Hierarchy& hierarchy = scene.getComponent<Hierarchy>(entity);
                cache.parent = hierarchy.parent;
                cache.childCount = hierarchy.childrenCount;

                if (!isValidNodeId(hierarchy.parent)) {
                    currentRoots.insert(entity);
                }
            } else {
                cache.parent = getInvalidNodeId();
                currentRoots.insert(entity);
            }

            auto oldIt = oldExpansionStates.find(entity);
            if (oldIt != oldExpansionStates.end()) {
                cache.isExpanded = oldIt->second;
            }

            m_nodeCache.emplace(entity, std::move(cache));
        });

        m_useLazyLoading = (m_totalNodeCount > LAZY_LOAD_THRESHOLD);

        for (Entity oldRoot: oldRootOrder) {
            if (currentRoots.count(oldRoot) > 0) {
                m_rootNodes.push_back(oldRoot);
                currentRoots.erase(oldRoot);
            }
        }

        for (Entity newRoot: currentRoots) {
            m_rootNodes.push_back(newRoot);
        }

        for (auto& [entity, cache]: m_nodeCache) {
            if (!scene.hasComponent<Hierarchy>(entity)) {
                continue;
            }

            const Hierarchy& hierarchy = scene.getComponent<Hierarchy>(entity);

            Entity childEntity = hierarchy.firstChild;
            while (isValidNodeId(childEntity)) {
                cache.children.push_back(childEntity);

                if (scene.hasComponent<Hierarchy>(childEntity)) {
                    childEntity = scene.getComponent<Hierarchy>(childEntity).nextSibling;
                } else {
                    break;
                }
            }
        }

        std::function<void(Entity, int)> calculateDepths = [this, &calculateDepths](Entity entity, int depth) {
            auto it = m_nodeCache.find(entity);
            if (it == m_nodeCache.end()) return;

            it->second.depth = depth;
            for (Entity child: it->second.children) {
                calculateDepths(child, depth + 1);
            }
        };

        for (Entity root: m_rootNodes) {
            calculateDepths(root, 0);
        }
    }

    std::string EntityHierarchyPanel::getNodeName(Entity id) const {
        Scene& scene = this->m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
        if (scene.hasComponent<Tag>(id)) {
            return scene.getComponent<Tag>(id).getName();
        }
        return "Entity " + std::to_string(id);
    }

    Entity EntityHierarchyPanel::getNodeParent(Entity id) const {
        Scene& scene = this->m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
        if (scene.hasComponent<Hierarchy>(id)) {
            return scene.getComponent<Hierarchy>(id).parent;
        }
        return getInvalidNodeId();
    }

    std::vector<Entity> EntityHierarchyPanel::getNodeChildren(Entity id) const {
        Scene& scene = this->m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
        return scene.getChildren(id);
    }

    void EntityHierarchyPanel::onSelectionChanged(Entity newSelection) {
        if (isValidNodeId(newSelection)) {
            auto it = m_nodeCache.find(newSelection);
            if (it != m_nodeCache.end()) {
                HierarchyNode node;
                node.type = HierarchyNode::NodeType::Entity;
                node.entity = newSelection;
                node.id = newSelection;
                node.name = it->second.name;
                node.depth = it->second.depth;
                node.isOpen = it->second.isExpanded;
                m_legacySelectedNode = node;
            }
        } else {
            m_legacySelectedNode = HierarchyNode();
        }
    }

    void EntityHierarchyPanel::drawEmptySpaceContextMenu() {
        if (m_config.allowCreation) {
            drawCreateEntityMenu("Create", getInvalidNodeId());
        }
    }

    void EntityHierarchyPanel::drawNodeContextMenu(Entity entity) {
        Scene& scene = getActiveScene();

        if (!scene.hasComponent<Tag>(entity)) {
            return;
        }

        Tag& tag = scene.getComponent<Tag>(entity);
        std::string name = tag.getName();

        if (m_config.allowCreation) {
            drawCreateEntityMenu("Create Child", entity);
            ImGui::Separator();
        }

        if (m_config.allowRenaming) {
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "Name", name)) {
                tag.setName(name);

                auto it = m_nodeCache.find(entity);
                if (it != m_nodeCache.end()) {
                    it->second.name = name;
                }
            }
            ImGui::Separator();
        }

        if (m_config.allowDeletion) {
            if (ImGui::MenuItem("Delete Entity")) {
                m_entitiesToDelete.push_back(entity);
            }
        }

        if (ImGui::MenuItem("Duplicate")) {
            m_entitiesToDuplicate.push_back(entity);
        }
    }

    void EntityHierarchyPanel::drawCreateEntityMenu(const std::string& title, Entity parent) {
        if (ImGui::BeginMenu(title.c_str())) {
            if (ImGui::MenuItem("Empty")) {
                m_entitiesToCreate.push_back({"New Entity", EntityType::Empty, parent});
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("3D Objects")) {
                if (ImGui::MenuItem("Cube")) {
                    m_entitiesToCreate.push_back({"Cube", EntityType::Cube, parent});
                }
                if (ImGui::MenuItem("Sphere")) {
                    m_entitiesToCreate.push_back({"Sphere", EntityType::Sphere, parent});
                }
                if (ImGui::MenuItem("Cylinder")) {
                    m_entitiesToCreate.push_back({"Cylinder", EntityType::Cylinder, parent});
                }
                if (ImGui::MenuItem("Capsule")) {
                    m_entitiesToCreate.push_back({"Capsule", EntityType::Capsule, parent});
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Camera")) {
                m_entitiesToCreate.push_back({"Camera", EntityType::Camera, parent});
            }

            if (ImGui::MenuItem("Light")) {
                m_entitiesToCreate.push_back({"Light", EntityType::Light, parent});
            }

            ImGui::EndMenu();
        }
    }

    void EntityHierarchyPanel::reparentNode(Entity entityToMove, Entity newParent, Entity insertBefore) {
        Scene& scene = getActiveScene();

        if (!scene.hasComponent<Hierarchy>(entityToMove)) {
            Hierarchy hierarchy;
            scene.addComponent<Hierarchy>(entityToMove, hierarchy);
        }

        // Get old parent to check if we're moving from one parent to another or reordering at root
        Entity oldParent = getInvalidNodeId();
        if (scene.hasComponent<Hierarchy>(entityToMove)) {
            oldParent = scene.getComponent<Hierarchy>(entityToMove).parent;
        }

        if (isValidNodeId(newParent)) {
            // Moving to a parent node
            scene.setParentPreservingWorldTransform(newParent, entityToMove);

            if (isValidNodeId(insertBefore) && scene.hasComponent<Hierarchy>(insertBefore)) {
                Hierarchy& parentHierarchy = scene.getComponent<Hierarchy>(newParent);
                Hierarchy& movedHierarchy = scene.getComponent<Hierarchy>(entityToMove);

                if (parentHierarchy.firstChild == entityToMove && movedHierarchy.nextSibling != insertBefore) {
                    parentHierarchy.firstChild = movedHierarchy.nextSibling;
                    if (isValidNodeId(movedHierarchy.nextSibling)) {
                        scene.getComponent<Hierarchy>(movedHierarchy.nextSibling).previousSibling = getInvalidNodeId();
                    }

                    Hierarchy& targetHierarchy = scene.getComponent<Hierarchy>(insertBefore);
                    movedHierarchy.nextSibling = insertBefore;
                    movedHierarchy.previousSibling = targetHierarchy.previousSibling;

                    if (isValidNodeId(targetHierarchy.previousSibling)) {
                        scene.getComponent<Hierarchy>(targetHierarchy.previousSibling).nextSibling = entityToMove;
                    } else {
                        parentHierarchy.firstChild = entityToMove;
                    }
                    targetHierarchy.previousSibling = entityToMove;
                }
            } else if (!isValidNodeId(insertBefore)) {
                Hierarchy& parentHierarchy = scene.getComponent<Hierarchy>(newParent);
                Hierarchy& movedHierarchy = scene.getComponent<Hierarchy>(entityToMove);

                if (parentHierarchy.firstChild == entityToMove && isValidNodeId(movedHierarchy.nextSibling)) {
                    Entity secondChild = movedHierarchy.nextSibling;
                    parentHierarchy.firstChild = secondChild;
                    scene.getComponent<Hierarchy>(secondChild).previousSibling = getInvalidNodeId();

                    Entity lastChild = secondChild;
                    while (scene.hasComponent<Hierarchy>(lastChild) &&
                           isValidNodeId(scene.getComponent<Hierarchy>(lastChild).nextSibling)) {
                        lastChild = scene.getComponent<Hierarchy>(lastChild).nextSibling;
                    }

                    scene.getComponent<Hierarchy>(lastChild).nextSibling = entityToMove;
                    movedHierarchy.previousSibling = lastChild;
                    movedHierarchy.nextSibling = getInvalidNodeId();
                }
            }

            // Remove from root list if it was there
            auto rootIt = std::find(m_rootNodes.begin(), m_rootNodes.end(), entityToMove);
            if (rootIt != m_rootNodes.end()) {
                m_rootNodes.erase(rootIt);
            }
        } else {
            // Moving to root level
            bool wasAlreadyRoot = !isValidNodeId(oldParent);

            // Unparent if currently has a parent
            if (isValidNodeId(oldParent)) {
                // Store world transform before unparenting
                glm::vec3 worldScale(1.0f);
                glm::quat worldRotation;
                glm::vec3 worldPosition(0.0f);

                if (scene.hasComponent<Transform>(entityToMove)) {
                    glm::mat4 worldMatrix = scene.getComponent<Transform>(entityToMove).m_worldMatrix;
                    glm::vec3 skew;
                    glm::vec4 perspective;
                    glm::decompose(worldMatrix, worldScale, worldRotation, worldPosition, skew, perspective);
                }

                scene.unParent(entityToMove);

                // Restore world transform as local (since no parent now, local = world)
                if (scene.hasComponent<Transform>(entityToMove)) {
                    Transform& transform = scene.getComponent<Transform>(entityToMove);
                    transform.m_position = worldPosition;
                    transform.m_rotation = glm::degrees(glm::eulerAngles(worldRotation));
                    transform.m_scale = worldScale;
                    transform.calculateUpForwardRight();
                    transform.m_isDirty = true;
                }
            }

            // Handle root-level ordering
            // First remove from current position in root list (if present)
            auto rootIt = std::find(m_rootNodes.begin(), m_rootNodes.end(), entityToMove);
            if (rootIt != m_rootNodes.end()) {
                m_rootNodes.erase(rootIt);
            }

            if (isValidNodeId(insertBefore)) {
                // Find insertBefore in root list and insert before it
                auto insertIt = std::find(m_rootNodes.begin(), m_rootNodes.end(), insertBefore);
                if (insertIt != m_rootNodes.end()) {
                    m_rootNodes.insert(insertIt, entityToMove);
                } else {
                    // insertBefore not found in roots, add to end
                    m_rootNodes.push_back(entityToMove);
                }
            } else {
                // No insertBefore specified, add to end
                m_rootNodes.push_back(entityToMove);
            }
        }
    }

    void EntityHierarchyPanel::processQueuedOperations() {
        if (!m_entitiesToDelete.empty()) {
            Scene& scene = getActiveScene();

            for (Entity entity: m_entitiesToDelete) {
                if (m_selectedNode == entity) {
                    clearSelection();
                    m_legacySelectedNode = HierarchyNode();
                }

                m_nodeCache.erase(entity);
                scene.destroyEntity(entity);
            }

            m_entitiesToDelete.clear();
            markFullRebuildNeeded();
        }

        if (!m_entitiesToDuplicate.empty()) {
            Scene& scene = getActiveScene();

            for (Entity entity: m_entitiesToDuplicate) {
                Entity duplicatedEntity = scene.duplicateEntity(entity);
                if (isValidNodeId(duplicatedEntity)) {
                    m_selectedNode = duplicatedEntity;
                }
            }

            m_entitiesToDuplicate.clear();
            markFullRebuildNeeded();
        }

        if (!m_entitiesToCreate.empty()) {
            for (const EntityToCreate& entityInfo: m_entitiesToCreate) {
                createEntity(entityInfo);
            }

            m_entitiesToCreate.clear();
            markFullRebuildNeeded();
        }
    }

    void EntityHierarchyPanel::createEntity(const EntityToCreate& entityInfo) {
        Scene& scene = getActiveScene();
        ResourceSystem& resourcesManager = m_appSystemsRegistry.getSystem<ResourceSystem>();

        Entity entityCreated = 0;

        switch (entityInfo.type) {
            case EntityType::Empty:
                entityCreated = scene.createEntity(entityInfo.name);
                break;

            case EntityType::Cube:
                entityCreated = scene.createEntity(entityInfo.name);
                scene.addComponent<MeshRenderer>(entityCreated,
                                                 ComponentsFactory::createMeshRenderer(
                                                     resourcesManager.getDefaultMesh().get(),
                                                     resourcesManager.getDefaultMaterial().get()));
                break;

            case EntityType::Sphere:
                entityCreated = scene.createEntity(entityInfo.name);
                scene.addComponent<MeshRenderer>(entityCreated,
                                                 ComponentsFactory::createMeshRenderer(
                                                     resourcesManager.getMeshResource("Sphere").get(),
                                                     resourcesManager.getDefaultMaterial().get()));
                break;

            case EntityType::Cylinder:
                entityCreated = scene.createEntity(entityInfo.name);
                break;

            case EntityType::Capsule:
                entityCreated = scene.createEntity(entityInfo.name);
                break;

            case EntityType::Camera:
                entityCreated = scene.createEntity(entityInfo.name);
                break;

            case EntityType::Light:
                entityCreated = scene.createEntity(entityInfo.name);
                break;
        }

        if (entityCreated != 0 && isValidNodeId(entityInfo.parent)) {
            scene.setParent(entityInfo.parent, entityCreated);
        }
    }

    void EntityHierarchyPanel::deleteEntity(Entity entity) {
        m_entitiesToDelete.push_back(entity);
    }
}
