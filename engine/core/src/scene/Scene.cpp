#include "Scene.hpp"

#include "core/Logger.hpp"
#include "core/UUID.hpp"

namespace TechEngine {
    void Scene::init() {
        nextEntityId = 0;
        freeEntities.clear();
    }

    Entity Scene::createEntity(const std::string& name) {
        int id;
        if (!freeEntities.empty()) {
            id = freeEntities.back();
            freeEntities.pop_back();
        } else {
            entities.push_back(nextEntityId);
            id = nextEntityId++;
        }
        std::string uuid = UUID::generate().toString();
        addComponent<Tag>(id, name, uuid);
        addComponent<Transform>(id);
        return id;
    }

    void Scene::destroyEntity(Entity entity) {
        freeEntities.push_back(entity);
        componentsManager.removeEntityComponents(entity);
    }

    const std::vector<Entity>& Scene::getEntities() const {
        return entities;
    }

    std::vector<std::any> Scene::getEntityComponents(Entity entity) {
        if (entity == -1) {
            TE_LOGGER_ERROR("Invalid entity id");
            return {};
        }
        return componentsManager.getEntityComponents(entity);
    }

    std::vector<std::string> Scene::getCommonComponents(const std::vector<Entity>& entities) {
        return componentsManager.getCommonComponents(entities);
    }
}
