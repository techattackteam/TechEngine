#include "Scene.hpp"

#include "components/Components.hpp"
#include "components/ComponentsFactory.hpp"
#include "core/UUID.hpp"

namespace TechEngine {
    Entity Scene::createEntity(const std::string& name) {
        UUID uuid = UUID::generate();
        Entity entity = m_archetypesManager.createEntity();
        m_archetypesManager.addComponent(entity, ComponentsFactory::createTag(name, uuid.toString()));
        m_archetypesManager.addComponent(entity, ComponentsFactory::createTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
        return entity;
    }

    void Scene::destroyEntity(Entity entity) {
        m_archetypesManager.removeEntity(entity);
    }

    std::vector<ComponentTypeID> Scene::getCommonComponents(const std::vector<Entity>& entities) {
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

    void Scene::setName(const std::string& name) {
        this->m_name = name;
    }

    const std::string& Scene::getName() const {
        return m_name;
    }

    int Scene::getTotalEntities() {
        size_t totalEntities = 0;
        for (std::unique_ptr<Archetype>& archetype: m_archetypesManager.m_archetypes) {
            totalEntities += archetype->getEntities().size();
        }
        return totalEntities;
    }
}
