#include "SceneInternal.hpp"
#include "eventSystem/EventManager.hpp"
#include "TechEngine/core/events/scene/ComponentAddedEvent.hpp"
#include "TechEngine/core/events/scene/ComponentRemovedEvent.hpp"
#include "TechEngine/core/events/scene/EntityCreatedEvent.hpp"
#include "TechEngine/core/events/scene/EntityDeletedEvent.hpp"

#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/components/ComponentsFactory.hpp"
#include "TechEngine/core/core/UUID.hpp"

namespace TechEngine {
    Scene::Scene(SystemsRegistry& systemsRegistry) : m_archetypesManager(), m_systemsRegistry(systemsRegistry), m_internal(std::make_unique<Internal>(m_archetypesManager)) {
    }

    Scene::~Scene() {
    }

    Entity Scene::createEntity(const std::string& name) {
        UUID uuid = UUID::generate();
        Entity entity = m_archetypesManager.createEntity();
        addComponent(entity, ComponentsFactory::createTag(name, uuid.toString()));
        addComponent(entity, ComponentsFactory::createTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
        m_systemsRegistry.getSystem<EventManager>().dispatch<EntityCreatedEvent>(entity);
        return entity;
    }

    void Scene::destroyEntity(Entity entity) {
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

    void Scene::clear() {
        m_archetypesManager.clear();
    }

    void Scene::Internal::setName(const std::string& name) {
        this->m_name = name;
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

    Scene::Internal::Internal(ArchetypesManager& archetypesManager) : m_archetypesManager(archetypesManager) {
    }

    int Scene::Internal::getTotalEntities() {
        size_t totalEntities = 0;
        for (std::unique_ptr<Archetype>& archetype: m_archetypesManager.m_archetypes) {
            totalEntities += archetype->getEntities().size();
        }
        return totalEntities;
    }
}
