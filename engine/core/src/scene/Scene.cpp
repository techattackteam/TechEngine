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

    int Scene::getEntityFromComponent(Camera* camera) {
        for (Entity entity: entities) {
            if (hasComponent<Camera>(entity)) {
                Camera& cameraComponent = getComponent<Camera>(entity);
                if (&cameraComponent == camera) {
                    return entity;
                }
            }
        }
        return -1;
    }

    bool Scene::hasMainCamera() {
        std::vector<Camera> cameras = getComponents<Camera>();
        for (Camera& camera: cameras) {
            if (camera.isMainCamera) {
                return true;
            }
        }
        return false;
    }

    Camera& Scene::getMainCamera() {
        assert(hasMainCamera() && "Main camera not found");
        std::vector<Camera> cameras = getComponents<Camera>();
        for (Camera& camera: cameras) {
            if (camera.isMainCamera) {
                return camera;
            }
        }
        TE_LOGGER_CRITICAL("Main camera not found and assert failed this should never happen");
        return cameras[0];
    }
}
