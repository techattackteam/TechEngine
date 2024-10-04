#include "scene/Scene.hpp"

#include "../../../core/src/scene/Scene.hpp"
#include "components/Components.hpp"
#include "components/Tag.hpp"
#include "components/Transform.hpp"
#include "components/MeshRenderer.hpp"
#include "resources/ResourcesManager.hpp"

namespace TechEngineAPI {
    void Scene::init(TechEngine::Scene* scene, TechEngine::ResourcesManager* resourcesManager) {
        Scene::m_scene = scene;
        Scene::m_resourcesManager = resourcesManager;
    }

    void Scene::shutdown() {
    }

    void Scene::updateComponents() {
        for (auto& [entity, component]: components) {
            component.get()->setEntity(entity);
            if (typeid(*component) == typeid(Tag)) {
                TechEngine::Tag& tag = m_scene->getComponent<TechEngine::Tag>(entity);
                dynamic_cast<Tag*>(component.get())->setName(tag.getName());
            } else if (typeid(*component) == typeid(Transform)) {
                TechEngine::Transform& transform = m_scene->getComponent<TechEngine::Transform>(entity);
                dynamic_cast<Transform*>(component.get())->setPosition(transform.position);
                dynamic_cast<Transform*>(component.get())->setRotation(transform.rotation);
                dynamic_cast<Transform*>(component.get())->setScale(transform.scale);
            } else if (typeid(*component) == typeid(MeshRenderer)) {
                TechEngine::MeshRenderer& meshRenderer = m_scene->getComponent<TechEngine::MeshRenderer>(entity);
                //std::shared_ptr<TechEngineAPI::Mesh> mesh = std::make_shared<TechEngineAPI::Mesh>(meshRenderer.mesh.m_name, m_resourcesManager);
                //dynamic_cast<MeshRenderer*>(component.get())->setMesh(mesh);

                //dynamic_cast<MeshRenderer*>(component.get())->setMesh(meshRenderer.mesh);
            }
        }
    }

    void Scene::sendUpdatedComponents() {
        //This kinda breaks the ECS patterns but for now it's fine
        for (auto& [entity, component]: components) {
            if (dynamic_cast<Tag*>(component.get())) {
                TechEngine::Tag& tag = m_scene->getComponent<TechEngine::Tag>(entity);
                tag.setName(dynamic_cast<Tag*>(component.get())->getName());
            } else if (dynamic_cast<Transform*>(component.get())) {
                TechEngine::Transform& transform = m_scene->getComponent<TechEngine::Transform>(entity);
                transform.position = dynamic_cast<Transform*>(component.get())->getPosition();
                transform.rotation = dynamic_cast<Transform*>(component.get())->getRotation();
                transform.scale = dynamic_cast<Transform*>(component.get())->getScale();
            } else if (dynamic_cast<MeshRenderer*>(component.get())) {
                if (dynamic_cast<MeshRenderer*>(component.get())->update) {
                    TechEngine::MeshRenderer& meshRenderer = m_scene->getComponent<TechEngine::MeshRenderer>(entity);
                    TechEngine::Mesh& mesh = m_resourcesManager->getMesh(dynamic_cast<MeshRenderer*>(component.get())->getMesh()->getName());
                    meshRenderer.changeMesh(mesh);
                    meshRenderer.paintMesh();
                    dynamic_cast<MeshRenderer*>(component.get())->update = false;
                }
            }
        }
    }

    Entity Scene::createEntity(const std::string& name) {
        return m_scene->createEntity(name);
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
            throw std::runtime_error("Entity already has component");
        } else {*/
            if (std::is_same<T, MeshRenderer>::value) {
                //static_assert(sizeof...(args) >= 2, "Uh-oh, too few args.");
                // Capture args in a tuple
                auto&& t = std::forward_as_tuple(args...);
                TechEngine::Mesh& mesh = m_resourcesManager->getMesh(std::get<0>(t)->getName());
                TechEngine::Material& material = m_resourcesManager->getMaterial(std::get<1>(t)->getName());
                TechEngine::MeshRenderer meshRenderer(mesh, material);
                m_scene->addComponent<TechEngine::MeshRenderer>(entity, meshRenderer);
                components[entity] = std::make_shared<MeshRenderer>(std::get<0>(t), std::get<1>(t));
            }
            return std::static_pointer_cast<T>(components[entity]);
        /*}*/
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
