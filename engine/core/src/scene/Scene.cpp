#include "SceneInternal.hpp"
#include "eventSystem/EventManager.hpp"
#include "TechEngine/core/events/scene/ComponentAddedEvent.hpp"
#include "TechEngine/core/events/scene/ComponentRemovedEvent.hpp"
#include "TechEngine/core/events/scene/EntityCreatedEvent.hpp"
#include "TechEngine/core/events/scene/EntityDeletedEvent.hpp"

#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/components/ComponentsFactory.hpp"
#include "TechEngine/core/core/UUID.hpp"

#include <glm/gtx/matrix_decompose.hpp>

namespace TechEngine {
    Scene::Scene(SystemsRegistry& systemsRegistry) : m_archetypesManager(), m_systemsRegistry(systemsRegistry), m_internal(std::make_unique<Internal>(*this, m_archetypesManager)) {
        ComponentsFactory::eventManager = &systemsRegistry.getSystem<EventManager>();
    }

    Scene::~Scene() {
    }

    Entity Scene::createEntity(const std::string& name) {
        UUID uuid = UUID::generate();
        Entity entity = m_archetypesManager.createEntity();
        addComponent(entity, ComponentsFactory::createTag(name, uuid.toString()));
        addComponent(entity, ComponentsFactory::createTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
        addComponent(entity, ComponentsFactory::createHierarchy());
        m_systemsRegistry.getSystem<EventManager>().dispatch<EntityCreatedEvent>(entity);
        return entity;
    }

    Entity Scene::duplicateEntity(Entity sourceEntity) {
        return duplicateEntityRecursive(sourceEntity, (Entity)-1);
    }

    Entity Scene::duplicateEntityRecursive(Entity sourceEntity, Entity newParent) {
        Entity newEntity = m_archetypesManager.duplicateEntity(sourceEntity);
        if (newEntity == (Entity)-1) {
            return -1; // Duplication failed
        }

        if (hasComponent<Tag>(newEntity)) {
            Tag& sourceTag = getComponent<Tag>(sourceEntity);
            UUID newUuid = UUID::generate();
            std::string newName = sourceTag.getName() + " (Copy)";
            // Since Tag's uuid is private, we'll create a new tag instead
            Tag duplicatedTag = ComponentsFactory::createTag(newName, newUuid.toString());
            getComponent<Tag>(newEntity) = duplicatedTag;
        }

        // Get the original parent before resetting hierarchy
        Entity originalParent = (Entity)-1;
        if (hasComponent<Hierarchy>(newEntity)) {
            originalParent = getComponent<Hierarchy>(newEntity).parent;
        }

        // Get the children of the source entity BEFORE resetting hierarchy
        std::vector<Entity> sourceChildren;
        if (hasComponent<Hierarchy>(sourceEntity)) {
            Hierarchy& sourceHierarchy = getComponent<Hierarchy>(sourceEntity);
            Entity child = sourceHierarchy.firstChild;
            while (child != (Entity)-1) {
                sourceChildren.push_back(child);
                if (hasComponent<Hierarchy>(child)) {
                    child = getComponent<Hierarchy>(child).nextSibling;
                } else {
                    break;
                }
            }
        }

        // Reset the Hierarchy component for the new entity
        if (hasComponent<Hierarchy>(newEntity)) {
            Hierarchy& newHierarchy = getComponent<Hierarchy>(newEntity);
            newHierarchy.parent = -1;
            newHierarchy.firstChild = -1;
            newHierarchy.nextSibling = -1;
            newHierarchy.previousSibling = -1;
            newHierarchy.childrenCount = 0;

            // Set the parent for the duplicate
            if (newParent != (Entity)-1) {
                // If a new parent was specified (for recursive calls), use that
                setParent(newParent, newEntity);
            } else if (originalParent != (Entity)-1) {
                // Otherwise, use the same parent as the source entity
                setParent(originalParent, newEntity);
            }
        }

        // Mark the transform as dirty so it gets updated
        if (hasComponent<Transform>(newEntity)) {
            Transform& newTransform = getComponent<Transform>(newEntity);
            newTransform.m_isDirty = true;
        }

        // Recursively duplicate all children
        for (Entity sourceChild : sourceChildren) {
            duplicateEntityRecursive(sourceChild, newEntity);
        }

        m_systemsRegistry.getSystem<EventManager>().dispatch<EntityCreatedEvent>(newEntity);
        return newEntity;
    }

    void Scene::destroyEntity(Entity entity) {
        Hierarchy& hierarchy = getComponent<Hierarchy>(entity);
        if (hierarchy.parent != -1) {
            unParent(entity);
        }

        Entity child = hierarchy.firstChild;
        while (child != -1) {
            Entity nextChild = getComponent<Hierarchy>(child).nextSibling;
            destroyEntity(child);
            child = nextChild;
        }

        m_archetypesManager.removeEntity(entity);
        m_systemsRegistry.getSystem<EventManager>().dispatch<EntityDeletedEvent>(entity);
    }

    std::vector<ComponentTypeID> Scene::Internal::getCommonComponents(const std::vector<Entity>& entities) {
        if (entities.empty()) {
            return {};
        }

        // Get the component types of the first entity
        std::vector<ComponentTypeID> commonComponents; {
            auto it = m_archetypesManager.m_entityToArchetypeMap.find(entities[0]);
            if (it == m_archetypesManager.m_entityToArchetypeMap.end()) {
                return {};
            }
            size_t archetypeIndex = it->second.m_archetypeIndex;
            Archetype* archetype = m_archetypesManager.m_archetypes[archetypeIndex].get();
            commonComponents = archetype->getComponentTypes();
        }

        // Intersect with the component types of the remaining entities
        for (size_t i = 1; i < entities.size(); ++i) {
            auto it = m_archetypesManager.m_entityToArchetypeMap.find(entities[i]);
            if (it == m_archetypesManager.m_entityToArchetypeMap.end()) {
                return {};
            }
            size_t archetypeIndex = it->second.m_archetypeIndex;
            Archetype* archetype = m_archetypesManager.m_archetypes[archetypeIndex].get();
            const std::vector<ComponentTypeID>& entityComponents = archetype->getComponentTypes();

            std::vector<ComponentTypeID> intersection;
            std::set_intersection(
                commonComponents.begin(), commonComponents.end(),
                entityComponents.begin(), entityComponents.end(),
                std::back_inserter(intersection)
            );
            commonComponents = intersection;

            if (commonComponents.empty()) {
                break; // No common components, exit early
            }
        }

        return commonComponents;
    }

    Entity Scene::getEntity(const Tag& tag) {
        return getEntity(tag.getUuid());
    }

    Entity Scene::getEntity(const std::string& uuid) {
        Entity entity = -1;
        m_archetypesManager.query<Entity, Tag>().each([&](Entity& e, Tag& tag) {
            if (tag.getUuid() == uuid) {
                entity = e;
            }
        });
        return entity;
    }

    void Scene::setParent(Entity parent, Entity child) {
        if (!hasComponent<Hierarchy>(child)) {
            TE_LOGGER_CRITICAL("Scene::unParent: Entity does not have Hierarchy component."); // Every entity should have Hierarchy component
            return;
        }
        if (child == parent) {
            return; // Prevent self-parenting
        }

        unParent(child);

        Hierarchy& childHierarchy = getComponent<Hierarchy>(child);
        Hierarchy& parentHierarchy = getComponent<Hierarchy>(parent);

        childHierarchy.parent = parent;
        childHierarchy.nextSibling = parentHierarchy.firstChild;
        childHierarchy.previousSibling = -1;

        if (parentHierarchy.firstChild != -1) {
            Hierarchy& firstChildHierarchy = getComponent<Hierarchy>(parentHierarchy.firstChild);
            firstChildHierarchy.previousSibling = child;
        }

        parentHierarchy.firstChild = child;
        parentHierarchy.childrenCount++;

        if (hasComponent<Transform>(child) && hasComponent<Transform>(parent)) {
            //getComponent<Transform>(child).m_isDirty = true;
        }
        TE_LOGGER_INFO("Scene::setParent: Parent set successfully. Parent: {0}, Child: {1}", parent, child);
    }

    void Scene::setParentPreservingWorldTransform(Entity parent, Entity child) {
        if (!hasComponent<Hierarchy>(child) || !hasComponent<Transform>(child)) {
            // Fall back to regular setParent if no transform
            setParent(parent, child);
            return;
        }

        if (child == parent) {
            return; // Prevent self-parenting
        }

        // Store the world transform before reparenting
        Transform& childTransform = getComponent<Transform>(child);
        glm::mat4 worldMatrix = childTransform.m_worldMatrix;

        // Decompose world matrix to get world position, rotation, scale
        glm::vec3 worldScale;
        glm::quat worldRotation;
        glm::vec3 worldPosition;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(worldMatrix, worldScale, worldRotation, worldPosition, skew, perspective);

        // Perform the reparenting
        setParent(parent, child);

        // Now convert world transform to local space relative to new parent
        if (parent != (Entity)-1 && hasComponent<Transform>(parent)) {
            Transform& parentTransform = getComponent<Transform>(parent);

            // Decompose parent's world matrix to get parent's world position, rotation, scale
            glm::vec3 parentWorldScale;
            glm::quat parentWorldRotation;
            glm::vec3 parentWorldPosition;
            glm::decompose(parentTransform.m_worldMatrix, parentWorldScale, parentWorldRotation, parentWorldPosition, skew, perspective);

            // Compute local scale: world scale / parent world scale
            glm::vec3 localScale = worldScale / parentWorldScale;

            // Compute local rotation: inverse of parent rotation * world rotation
            glm::quat parentRotationInverse = glm::inverse(parentWorldRotation);
            glm::quat localRotation = parentRotationInverse * worldRotation;

            // Compute local position:
            // The world position is computed as: parentWorldPos + parentWorldRot * (parentWorldScale * localPos)
            // So: localPos = (inverse(parentWorldRot) * (worldPos - parentWorldPos)) / parentWorldScale
            glm::vec3 relativePos = worldPosition - parentWorldPosition;
            glm::vec3 rotatedRelativePos = parentRotationInverse * relativePos;
            glm::vec3 localPosition = rotatedRelativePos / parentWorldScale;

            // Apply local transform
            childTransform.m_position = localPosition;
            childTransform.m_rotation = glm::degrees(glm::eulerAngles(localRotation));
            childTransform.m_scale = localScale;
            childTransform.calculateUpForwardRight();
        } else {
            // Parent is root, use world transform directly
            childTransform.m_position = worldPosition;
            childTransform.m_rotation = glm::degrees(glm::eulerAngles(worldRotation));
            childTransform.m_scale = worldScale;
            childTransform.calculateUpForwardRight();
        }

        childTransform.m_isDirty = true;
    }

    void Scene::unParent(Entity child) {
        if (!hasComponent<Hierarchy>(child)) {
            TE_LOGGER_CRITICAL("Scene::unParent: Entity does not have Hierarchy component."); // Every entity should have Hierarchy component
            return;
        }

        Hierarchy& childHierarchy = getComponent<Hierarchy>(child);
        Entity parent = childHierarchy.parent;

        if (parent == -1) {
            return; // Already unparented
        }

        Hierarchy& parentHierarchy = getComponent<Hierarchy>(parent);

        if (childHierarchy.previousSibling != -1) {
            m_archetypesManager.getComponent<Hierarchy>(childHierarchy.previousSibling).nextSibling = childHierarchy.nextSibling;
        } else {
            parentHierarchy.firstChild = childHierarchy.nextSibling;
        }

        if (childHierarchy.nextSibling != -1) {
            m_archetypesManager.getComponent<Hierarchy>(childHierarchy.nextSibling).previousSibling = childHierarchy.previousSibling;
        }

        childHierarchy.parent = -1;
        childHierarchy.nextSibling = -1;
        childHierarchy.previousSibling = -1;

        if (parentHierarchy.childrenCount > 0) parentHierarchy.childrenCount--;

        if (hasComponent<Transform>(child)) {
            //getComponent<Transform>(child).m_isDirty = true;
        }
    }

    std::vector<Entity> Scene::getChildren(Entity parent) {
        std::vector<Entity> children;
        if (!hasComponent<Hierarchy>(parent)) {
            TE_LOGGER_CRITICAL("Scene::getChildren: Entity does not have Hierarchy component."); // Every entity should have Hierarchy component
            return children;
        }

        Hierarchy& parentHierarchy = getComponent<Hierarchy>(parent);
        Entity currentChild = parentHierarchy.firstChild;

        while (currentChild != -1) {
            children.push_back(currentChild);
            Hierarchy& currentChildHierarchy = getComponent<Hierarchy>(currentChild);
            currentChild = currentChildHierarchy.nextSibling;
        }

        return children;
    }

    void Scene::clear() {
        m_archetypesManager.clear();
    }

    void Scene::Internal::setName(const std::string& name) {
        this->m_name = name;
    }

    void Scene::Internal::updateGlobalTransforms() {
        m_archetypesManager.query<Entity, Transform, Hierarchy>().each([this](Entity entity, Transform& transform, Hierarchy& hierarchy) {
            if (hierarchy.parent == -1) {
                // This is a root node
                updateTransformsRecursive(entity, glm::mat4(1.0f), glm::vec3(1.0f), false);
            }
        });
    }

    void Scene::Internal::updateTransformsRecursive(Entity entity, const glm::mat4& parentTransformNoScale, const glm::vec3& parentWorldScale, bool parentDirty) {
        if (!m_scene.hasComponent<Transform>(entity)) {
            return;
        }

        Transform& transform = m_scene.getComponent<Transform>(entity);
        glm::mat4 localRotation = glm::mat4_cast(glm::quat(glm::radians(transform.m_rotation)));
        glm::vec3 scaledLocalPos = parentWorldScale * transform.m_position;
        glm::mat4 worldRotationTranslation = parentTransformNoScale *
                                             glm::translate(glm::mat4(1.0f), scaledLocalPos) *
                                             localRotation;

        glm::vec3 finalWorldScale = transform.m_scale;

        transform.m_worldMatrix = worldRotationTranslation * glm::scale(glm::mat4(1.0f), finalWorldScale);

        transform.m_isDirty = false;
        parentDirty = true;

        glm::mat4 matrixToPassNoScale = worldRotationTranslation;

        Hierarchy& hierarchy = m_scene.getComponent<Hierarchy>(entity);
        Entity child = hierarchy.firstChild;
        while (child != -1) {
            updateTransformsRecursive(child, matrixToPassNoScale, finalWorldScale, parentDirty);

            Hierarchy& childHierarchy = m_scene.getComponent<Hierarchy>(child);
            child = childHierarchy.nextSibling;
        }
    }

    const std::string& Scene::getName() const {
        return m_internal->m_name;
    }

    std::unique_ptr<Scene::Internal>& Scene::getInternal() {
        return m_internal;
    }

    void Scene::addComponentInternal(Entity entity, ComponentTypeID componentTypeID) {
        m_systemsRegistry.getSystem<EventManager>().dispatch<ComponentAddedEvent>(entity, componentTypeID);
    }

    void Scene::removeComponentInternal(Entity entity, ComponentTypeID componentTypeID) {
        m_systemsRegistry.getSystem<EventManager>().dispatch<ComponentRemovedEvent>(entity, componentTypeID);
    }

    Scene::Internal::Internal(Scene& scene, ArchetypesManager& archetypesManager) : m_scene(scene), m_archetypesManager(archetypesManager) {
    }


    int Scene::Internal::getTotalEntities() {
        size_t totalEntities = 0;
        for (std::unique_ptr<Archetype>& archetype: m_archetypesManager.m_archetypes) {
            totalEntities += archetype->getEntities().size();
        }
        return totalEntities;
    }
}
