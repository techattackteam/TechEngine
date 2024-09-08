#include "Scene.hpp"

#include "components/Components.hpp"
#include "core/Logger.hpp"
#include "core/UUID.hpp"

namespace TechEngine {
    void Scene::init() {
    }

    Entity Scene::createEntity(const std::string& name) {
        Entity entity = archetypesManager.createEntity();
        std::string uuid = UUID::generate().toString();
        archetypesManager.addComponent(entity, Tag(name, uuid));
        archetypesManager.addComponent(entity, Transform());
        return entity;
    }

    void Scene::destroyEntity(Entity entity) {
        archetypesManager.removeEntity(entity);
    }

    std::vector<Archetype> Scene::queryArchetypes(const std::vector<ComponentTypeID>& requiredComponents) {
        return archetypesManager.queryArchetypes(requiredComponents);
    }

    std::vector<Entity> Scene::getEntities() {
        std::vector<Archetype> archetypes = queryArchetypes({ComponentType::get<Tag>()});
        std::vector<Entity> entities;
        for (const Archetype& archetype: archetypes) {
            for (Entity entity: archetype.getEntities()) {
                entities.push_back(entity);
            }
        }
        return entities;
    }

    std::vector<char> Scene::getEntityComponents(Entity entity) {
        return archetypesManager.getEntityComponents(entity);
    }

    std::vector<ComponentTypeID> Scene::getCommonComponents(const std::vector<Entity>& entities) {
        std::vector<ComponentTypeID> commonComponents;
        for (Entity entity: entities) {
            std::vector<ComponentTypeID> entityArchetype = archetypesManager.getComponentTypes(entity);
            for (ComponentTypeID componentType: entityArchetype) {
                if (std::find(commonComponents.begin(), commonComponents.end(), componentType) == commonComponents.end()) {
                    commonComponents.push_back(componentType);
                }
            }
        }
        return commonComponents;
    }

    Entity Scene::getEntityByTag(const Tag& tag) {
        std::vector<Archetype> archetypes = queryArchetypes({ComponentType::get<Tag>()});
        for (const Archetype& archetype: archetypes) {
            for (Entity entity: archetype.getEntities()) {
                Tag& entityTag = archetypesManager.getComponent<Tag>(entity);
                if (entityTag == tag) {
                    return entity;
                }
            }
        }
        return -1;
    }
}
