#include "scene/Scene.hpp"

#include "../../../core/src/scene/Scene.hpp"
#include "components/Components.hpp"
#include "components/Tag.hpp"
#include "components/Transform.hpp"
#include "components/MeshRenderer.hpp"
#include "resources/Resources.hpp"
#include "resources/ResourcesManager.hpp"

namespace TechEngineAPI {
    void Scene::init(TechEngine::Scene* scene, TechEngine::ResourcesManager* resourcesManager) {
        Scene::m_scene = scene;
        Scene::m_resourcesManager = resourcesManager;
    }

    void Scene::shutdown() {
    }

    Entity Scene::createEntity(const std::string& name) {
        Entity entity = m_scene->createEntity(name);
        std::shared_ptr<Tag> tagComponent = std::make_shared<Tag>(entity, name);
        components[{entity, typeid(Tag)}] = tagComponent;

        std::shared_ptr<Transform> transformComponent = std::make_shared<Transform>(entity, &m_scene->getComponent<TechEngine::Transform>(entity));
        components[{entity, typeid(Transform)}] = transformComponent;
        return entity;
    }

    std::vector<Entity> Scene::getEntitiesWithName(const std::string& name) {
        std::vector<Entity> entities;
        m_scene->runSystem<TechEngine::Tag>([&entities, name](TechEngine::Tag& tag) {
            if (tag.getName() == name) {
                entities.push_back(m_scene->getEntityByTag(tag));
            }
        });
        return entities;
    }

    void Scene::destroyEntity(Entity entity) {
        m_scene->destroyEntity(entity);
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> Scene::addComponent(Entity entity, Args&&... args) {
        return addComponentInternal<T>(entity, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> Scene::addComponentInternal(Entity entity, Args&&... args) {
        /*if (m_scene->hasComponent<T>(entity)) {
            TE_LOGGER_WARN("Entity already has component");
        } else {*/
        if (std::is_same<T, MeshRenderer>::value) {
            auto&& t = std::forward_as_tuple(args...);
            TechEngine::Mesh& mesh = m_resourcesManager->getMesh(std::get<0>(t)->getName());
            TechEngine::Material& material = m_resourcesManager->getMaterial(std::get<1>(t)->getName());
            TechEngine::MeshRenderer meshRenderer(mesh, material);
            m_scene->addComponent<TechEngine::MeshRenderer>(entity, meshRenderer);
            components[{entity, typeid(MeshRenderer)}] = std::make_shared<MeshRenderer>(entity, std::get<0>(t), std::get<1>(t));
        }
        /*}*/
        for (auto&& component: components) {
            component.second->updateInternalPointer(m_scene);
        }
        return std::static_pointer_cast<T>(components[{entity, typeid(T)}]);
    }

    template<typename T>
    std::shared_ptr<T> Scene::getComponent(Entity entity) {
        if (std::is_same<T, Tag>::value) {
            std::shared_ptr<Tag> component = std::static_pointer_cast<Tag>(components[{entity, typeid(Tag)}]);
            TechEngine::Tag& tag = getComponentInternal<TechEngine::Tag>(entity);
            component->setName(tag.getName());
            return std::static_pointer_cast<T>(components[{entity, typeid(Tag)}]);
        } else if (std::is_same<T, Transform>::value) {
            std::shared_ptr<Transform> component = std::static_pointer_cast<Transform>(components[{entity, typeid(Transform)}]);
            /*TechEngine::Transform& transform = getComponentInternal<TechEngine::Transform>(entity);
            component->setPosition(transform.position);
            component->setRotation(transform.rotation);
            component->setScale(transform.scale);*/
            return std::static_pointer_cast<T>(components[{entity, typeid(Transform)}]);
        } else if (std::is_same<T, MeshRenderer>::value) {
            std::shared_ptr<MeshRenderer> component = std::static_pointer_cast<MeshRenderer>(components[{entity, typeid(MeshRenderer)}]);
            TechEngine::MeshRenderer& meshRenderer = getComponentInternal<TechEngine::MeshRenderer>(entity);
            std::shared_ptr<Mesh> mesh = Resources::getMesh(meshRenderer.mesh.getName());
            std::shared_ptr<Material> material = Resources::getMaterial(meshRenderer.material.getName());
            component->setMesh(mesh);
            component->setMaterial(material);
            return std::static_pointer_cast<T>(components[{entity, typeid(MeshRenderer)}]);
        } else {
            throw std::runtime_error("Entity does not have component");
        }
    }

    template<typename T>
    T& Scene::getComponentInternal(Entity entity) {
        if (m_scene->hasComponent<T>(entity)) {
            return m_scene->getComponent<T>(entity);
        } else {
            throw std::runtime_error("Entity does not have component");
        }
    }

    template API_DLL std::shared_ptr<MeshRenderer> Scene::addComponent<MeshRenderer, std::shared_ptr<Mesh>&, std::shared_ptr<Material>&>(Entity, std::shared_ptr<Mesh>&, std::shared_ptr<Material>&);

    template API_DLL std::shared_ptr<MeshRenderer> Scene::addComponentInternal<MeshRenderer, std::shared_ptr<Mesh>&, std::shared_ptr<Material>&>(Entity, std::shared_ptr<Mesh>&, std::shared_ptr<Material>&);

    template API_DLL std::shared_ptr<Tag> Scene::getComponent<Tag>(Entity entity);

    template API_DLL std::shared_ptr<Transform> Scene::getComponent<Transform>(Entity entity);

    template API_DLL std::shared_ptr<MeshRenderer> Scene::getComponent<MeshRenderer>(Entity entity);

    template API_DLL Tag& Scene::getComponentInternal<Tag>(Entity entity);

    template API_DLL Transform& Scene::getComponentInternal<Transform>(Entity entity);

    template API_DLL MeshRenderer& Scene::getComponentInternal<MeshRenderer>(Entity entity);
}
