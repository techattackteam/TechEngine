#include "scene/Scene.hpp"

#include "../../../core/src/scene/Scene.hpp"
#include "components/Components.hpp"
#include "components/Tag.hpp"
#include "components/Transform.hpp"

namespace TechEngineAPI {
    void Scene::init(TechEngine::Scene* scene) {
        Scene::scene = scene;
    }

    void Scene::shutdown() {
    }

    void Scene::updateComponents() {
        for (auto& [entity, component]: components) {
            component.get()->setEntity(entity);
            if (typeid(*component) == typeid(Tag)) {
                TechEngine::Tag& tag = scene->getComponent<TechEngine::Tag>(entity);
                static_cast<Tag*>(component.get())->setName(tag.getName());
            } else if (typeid(*component) == typeid(Transform)) {
                TechEngine::Transform& transform = scene->getComponent<TechEngine::Transform>(entity);
                static_cast<Transform*>(component.get())->setPosition(transform.position);
                static_cast<Transform*>(component.get())->setRotation(transform.rotation);
                static_cast<Transform*>(component.get())->setScale(transform.scale);
            }
        }
    }

    void Scene::sendUpdatedComponents() {
        //This kinda breaks the ECS patterns but for now it's fine
        for (auto& [entity, component]: components) {
            if (dynamic_cast<Tag*>(component.get())) {
                TechEngine::Tag& tag = scene->getComponent<TechEngine::Tag>(entity);
                tag.setName(static_cast<Tag*>(component.get())->getName());
            } else if (dynamic_cast<Transform*>(component.get())) {
                TechEngine::Transform& transform = scene->getComponent<TechEngine::Transform>(entity);
                transform.position = static_cast<Transform*>(component.get())->getPosition();
                transform.rotation = static_cast<Transform*>(component.get())->getRotation();
                transform.scale = static_cast<Transform*>(component.get())->getScale();
            }
        }
    }

    Entity Scene::createEntity(const std::string& name) {
        return scene->createEntity(name);
    }

    std::vector<Entity> Scene::getEntitiesWithName(const std::string& name) {
        std::vector<Entity> entities;
        scene->runSystem<TechEngine::Tag>([&entities, name](TechEngine::Tag& tag) {
            if (tag.getName() == name) {
                entities.push_back(scene->getEntityByTag(tag));
            }
        });
        return entities;
    }

    void Scene::destroyEntity(Entity entity) {
        scene->destroyEntity(entity);
    }

    template<typename T>
    std::shared_ptr<T> Scene::getComponent(Entity entity) {
        if (std::is_same<T, Tag>::value) {
            TechEngine::Tag& tag = getComponentInternal<TechEngine::Tag>(entity);
            components[entity] = std::make_shared<Tag>(tag.getName());
            return std::static_pointer_cast<T>(components[entity]);
        } else if (std::is_same<T, Transform>::value) {
            TechEngine::Transform& transform = getComponentInternal<TechEngine::Transform>(entity);
            components[entity] = std::make_shared<Transform>(transform.position, transform.rotation, transform.scale);
            return std::static_pointer_cast<T>(components[entity]);
        } else {
            throw std::runtime_error("Entity does not have component");
        }
    }

    template<typename T>
    T& Scene::getComponentInternal(Entity entity) {
        if (scene->hasComponent<T>(entity)) {
            return scene->getComponent<T>(entity);
        } else {
            throw std::runtime_error("Entity does not have component");
        }
    }

    template API_DLL std::shared_ptr<Tag> Scene::getComponent<Tag>(Entity entity);

    template API_DLL std::shared_ptr<Transform> Scene::getComponent<Transform>(Entity entity);

    template API_DLL Tag& Scene::getComponentInternal<Tag>(Entity entity);

    template API_DLL Transform& Scene::getComponentInternal<Transform>(Entity entity);
}
