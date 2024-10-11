#include "Scene.hpp"

#include "components/Components.hpp"
#include "components/ComponentsFactory.hpp"
#include "core/UUID.hpp"

namespace TechEngine {
    Entity Scene::createEntity(const std::string& name) {
        std::string uuid = UUID::generate().toString();
        return createEntity(name, uuid);
    }

    void Scene::destroyEntity(Entity entity) {
        m_archetypesManager.removeEntity(entity);
    }

    std::vector<ComponentTypeID> Scene::getCommonComponents(const std::vector<Entity>& entities) {
        std::vector<ComponentTypeID> commonComponents;
        for (Entity entity: entities) {
            std::vector<ComponentTypeID> entityArchetype = m_archetypesManager.getComponentTypes(entity);
            for (ComponentTypeID componentType: entityArchetype) {
                if (std::find(commonComponents.begin(), commonComponents.end(), componentType) == commonComponents.end()) {
                    commonComponents.push_back(componentType);
                }
            }
        }
        return commonComponents;
    }

    Entity Scene::getEntityByTag(const Tag& tag) {
        std::vector<Archetype*> archetypes = m_archetypesManager.queryArchetypes({ComponentType::get<Tag>()});
        for (Archetype* archetype: archetypes) {
            std::vector<Tag>& tags = archetype->getComponentArray<Tag>();
            for (Tag& t: tags) {
                if (t == tag) {
                    return archetype->getEntities()[&t - &tags[0]];
                }
            }
        }
        return -1;
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

    Entity Scene::createEntity(const std::string& name, const std::string& uuid) {
        Entity entity = m_archetypesManager.createEntity();
        m_archetypesManager.addComponent(entity, ComponentsFactory::createTag(name, uuid));
        m_archetypesManager.addComponent(entity, ComponentsFactory::createTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
        return entity;
    }
}
