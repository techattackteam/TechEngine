#include "common/include/components/Tag.hpp"
#include "common/include/components/Transform.hpp"
#include "common/include/components/render/MeshRenderer.hpp"
#include "common/include/components/physics/BoxCollider.hpp"
#include "common/include/resources/Resources.hpp"
#include "common/include/scene/Scene.hpp"

#include "scene/Scene.hpp"

#include "common/include/components/physics/CapsuleCollider.hpp"
#include "common/include/components/physics/CylinderCollider.hpp"
#include "common/include/components/physics/SphereCollider.hpp"
#include "components/Components.hpp"
#include "components/ComponentsFactory.hpp"
#include "resources/ResourcesManager.hpp"

namespace TechEngineAPI {
    void Scene::init(TechEngine::Scene* scene, TechEngine::ResourcesManager* resourcesManager, TechEngine::PhysicsEngine* physicsEngine) {
        m_scene = scene;
        m_resourcesManager = resourcesManager;
        m_physicsEngine = physicsEngine;

        components.clear();
        m_scene->runSystem<TechEngine::Tag>([&](TechEngine::Tag& tag) {
            Entity entity = m_scene->getEntityByTag(tag);
            std::shared_ptr<Tag> tagComponent = std::make_shared<Tag>(entity, m_scene->getComponent<TechEngine::Tag>(entity).getName());
            components[{entity, typeid(Tag)}] = tagComponent;

            std::shared_ptr<Transform> transformComponent = std::make_shared<Transform>(entity, &m_scene->getComponent<TechEngine::Transform>(entity));
            components[{entity, typeid(Transform)}] = transformComponent;

            if (m_scene->hasComponent<TechEngine::MeshRenderer>(entity)) {
                const auto& meshRenderer = m_scene->getComponent<TechEngine::MeshRenderer>(entity);
                std::shared_ptr<Mesh> mesh = Resources::getMesh(meshRenderer.mesh.getName());
                std::shared_ptr<Material> material = Resources::getMaterial(meshRenderer.material.getName());
                std::shared_ptr<MeshRenderer> meshRendererComponent = std::make_shared<MeshRenderer>(entity, mesh, material);
                components[{entity, typeid(MeshRenderer)}] = meshRendererComponent;
            }

            if (m_scene->hasComponent<TechEngine::BoxCollider>(entity)) {
                const auto& boxCollider = m_scene->getComponent<TechEngine::BoxCollider>(entity);
                const std::shared_ptr<BoxCollider> boxColliderComponent = std::make_shared<BoxCollider>(entity);
                boxColliderComponent->updateInternalPointer(m_scene);
                boxColliderComponent->setCenter(boxCollider.center);
                boxColliderComponent->setSize(boxCollider.size);
                components[{entity, typeid(BoxCollider)}] = boxColliderComponent;
            }
            if (m_scene->hasComponent<TechEngine::SphereCollider>(entity)) {
                const auto& sphereCollider = m_scene->getComponent<TechEngine::SphereCollider>(entity);
                const std::shared_ptr<SphereCollider> sphereColliderComponent = std::make_shared<SphereCollider>(entity);
                sphereColliderComponent->updateInternalPointer(m_scene);
                sphereColliderComponent->setCenter(sphereCollider.center);
                sphereColliderComponent->setRadius(sphereCollider.radius);
                components[{entity, typeid(SphereCollider)}] = sphereColliderComponent;
            }
            if (m_scene->hasComponent<TechEngine::CapsuleCollider>(entity)) {
                const auto& capsuleCollider = m_scene->getComponent<TechEngine::CapsuleCollider>(entity);
                const std::shared_ptr<CapsuleCollider> capsuleColliderComponent = std::make_shared<CapsuleCollider>(entity);
                capsuleColliderComponent->updateInternalPointer(m_scene);
                capsuleColliderComponent->setCenter(capsuleCollider.center);
                capsuleColliderComponent->setHeight(capsuleCollider.height);
                capsuleColliderComponent->setRadius(capsuleCollider.radius);
                components[{entity, typeid(CapsuleCollider)}] = capsuleColliderComponent;
            }
            if (m_scene->hasComponent<TechEngine::CylinderCollider>(entity)) {
                const auto& cylinderCollider = m_scene->getComponent<TechEngine::CylinderCollider>(entity);
                const std::shared_ptr<CylinderCollider> cylinderColliderComponent = std::make_shared<CylinderCollider>(entity);
                cylinderColliderComponent->updateInternalPointer(m_scene);
                cylinderColliderComponent->setCenter(cylinderCollider.center);
                cylinderColliderComponent->setHeight(cylinderCollider.height);
                cylinderColliderComponent->setRadius(cylinderCollider.radius);
                components[{entity, typeid(CylinderCollider)}] = cylinderColliderComponent;
            }
        });
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
        if (components.contains({entity, typeid(T)})) {
            TE_LOGGER_WARN("Entity already has component");
        } else {
            if constexpr (std::is_same_v<T, MeshRenderer>) {
                auto&& t = std::forward_as_tuple(args...);
                TechEngine::Mesh& mesh = m_resourcesManager->getMesh(std::get<0>(t)->getName());
                TechEngine::Material& material = m_resourcesManager->getMaterial(std::get<1>(t)->getName());
                TechEngine::MeshRenderer meshRenderer(mesh, material);
                m_scene->addComponent<TechEngine::MeshRenderer>(entity, meshRenderer);
                components[{entity, typeid(MeshRenderer)}] = std::make_shared<MeshRenderer>(entity, std::get<0>(t), std::get<1>(t));
            } else if constexpr (std::is_same_v<T, BoxCollider>) {
                const TechEngine::Tag& tag = m_scene->getComponent<TechEngine::Tag>(entity);
                const TechEngine::Transform& transform = m_scene->getComponent<TechEngine::Transform>(entity);
                m_scene->addComponent<TechEngine::BoxCollider>(entity,
                                                               TechEngine::ComponentsFactory::createBoxCollider(*m_physicsEngine,
                                                                                                                tag,
                                                                                                                transform,
                                                                                                                glm::vec3(0.0f),
                                                                                                                glm::vec3(1.0f)));
                components[{entity, typeid(BoxCollider)}] = std::make_shared<BoxCollider>(entity);
            } else if constexpr (std::is_same_v<T, SphereCollider>) {
                const TechEngine::Tag& tag = m_scene->getComponent<TechEngine::Tag>(entity);
                const TechEngine::Transform& transform = m_scene->getComponent<TechEngine::Transform>(entity);
                m_scene->addComponent<TechEngine::SphereCollider>(entity,
                                                                  TechEngine::ComponentsFactory::createSphereCollider(*m_physicsEngine,
                                                                                                                      tag,
                                                                                                                      transform,
                                                                                                                      glm::vec3(0.0f),
                                                                                                                      0.5f));
                components[{entity, typeid(SphereCollider)}] = std::make_shared<SphereCollider>(entity);
            } else if constexpr (std::is_same_v<T, CapsuleCollider>) {
                const TechEngine::Tag& tag = m_scene->getComponent<TechEngine::Tag>(entity);
                const TechEngine::Transform& transform = m_scene->getComponent<TechEngine::Transform>(entity);
                m_scene->addComponent<TechEngine::CapsuleCollider>(entity,
                                                                   TechEngine::ComponentsFactory::createCapsuleCollider(*m_physicsEngine,
                                                                                                                        tag,
                                                                                                                        transform,
                                                                                                                        glm::vec3(0.0f),
                                                                                                                        0.5f,
                                                                                                                        1.0f));
                components[{entity, typeid(CapsuleCollider)}] = std::make_shared<CapsuleCollider>(entity);
            } else if constexpr (std::is_same_v<T, CylinderCollider>) {
                const TechEngine::Tag& tag = m_scene->getComponent<TechEngine::Tag>(entity);
                const TechEngine::Transform& transform = m_scene->getComponent<TechEngine::Transform>(entity);
                m_scene->addComponent<TechEngine::CylinderCollider>(entity,
                                                                    TechEngine::ComponentsFactory::createCylinderCollider(*m_physicsEngine,
                                                                                                                          tag,
                                                                                                                          transform,
                                                                                                                          glm::vec3(0.0f),
                                                                                                                          0.5f,
                                                                                                                          1.0f));
                components[{entity, typeid(CylinderCollider)}] = std::make_shared<CylinderCollider>(entity);
            } else {
                throw std::runtime_error("Component not supported");
            }
        }
        for (auto&& component: components) {
            component.second->updateInternalPointer(m_scene);
        }
        return std::static_pointer_cast<T>(components[{entity, typeid(T)}]);
    }

    template<typename T>
    std::shared_ptr<T> Scene::getComponent(const Entity entity) {
        if constexpr (std::is_same_v<T, Tag>) {
            std::shared_ptr<Tag> component = std::static_pointer_cast<Tag>(components[{entity, typeid(Tag)}]);
            TechEngine::Tag& tag = getComponentInternal<TechEngine::Tag>(entity);
            component->setName(tag.getName());
            return std::static_pointer_cast<T>(components[{entity, typeid(Tag)}]);
        } else if constexpr (std::is_same_v<T, Transform>) {
            std::shared_ptr<Transform> component = std::static_pointer_cast<Transform>(components[{entity, typeid(Transform)}]);
            TechEngine::Transform& transform = getComponentInternal<TechEngine::Transform>(entity);
            component->setPosition(transform.position);
            component->setRotation(transform.rotation);
            component->setScale(transform.scale);
            return std::static_pointer_cast<T>(components[{entity, typeid(Transform)}]);
        } else if constexpr (std::is_same_v<T, MeshRenderer>) {
            std::shared_ptr<MeshRenderer> component = std::static_pointer_cast<MeshRenderer>(components[{entity, typeid(MeshRenderer)}]);
            TechEngine::MeshRenderer& meshRenderer = getComponentInternal<TechEngine::MeshRenderer>(entity);
            std::shared_ptr<Mesh> mesh = Resources::getMesh(meshRenderer.mesh.getName());
            std::shared_ptr<Material> material = Resources::getMaterial(meshRenderer.material.getName());
            component->setMesh(mesh);
            component->setMaterial(material);
            return std::static_pointer_cast<T>(components[{entity, typeid(MeshRenderer)}]);
        } else if constexpr (std::is_same_v<T, BoxCollider>) {
            std::shared_ptr<BoxCollider> component = std::static_pointer_cast<BoxCollider>(components[{entity, typeid(BoxCollider)}]);
            TechEngine::BoxCollider& boxCollider = getComponentInternal<TechEngine::BoxCollider>(entity);
            component->setCenter(boxCollider.center);
            component->setSize(boxCollider.size);
            return std::static_pointer_cast<T>(components[{entity, typeid(BoxCollider)}]);
        } else if constexpr (std::is_same_v<T, SphereCollider>) {
            std::shared_ptr<SphereCollider> component = std::static_pointer_cast<SphereCollider>(components[{entity, typeid(SphereCollider)}]);
            TechEngine::SphereCollider& sphereCollider = getComponentInternal<TechEngine::SphereCollider>(entity);
            component->setCenter(sphereCollider.center);
            component->setRadius(sphereCollider.radius);
            return std::static_pointer_cast<T>(components[{entity, typeid(SphereCollider)}]);
        } else if constexpr (std::is_same_v<T, CapsuleCollider>) {
            std::shared_ptr<CapsuleCollider> component = std::static_pointer_cast<CapsuleCollider>(components[{entity, typeid(CapsuleCollider)}]);
            TechEngine::CapsuleCollider& capsuleCollider = getComponentInternal<TechEngine::CapsuleCollider>(entity);
            component->setCenter(capsuleCollider.center);
            component->setHeight(capsuleCollider.height);
            component->setRadius(capsuleCollider.radius);
            return std::static_pointer_cast<T>(components[{entity, typeid(CapsuleCollider)}]);
        } else if constexpr (std::is_same_v<T, CylinderCollider>) {
            std::shared_ptr<CylinderCollider> component = std::static_pointer_cast<CylinderCollider>(components[{entity, typeid(CylinderCollider)}]);
            TechEngine::CylinderCollider& cylinderCollider = getComponentInternal<TechEngine::CylinderCollider>(entity);
            component->setCenter(cylinderCollider.center);
            component->setHeight(cylinderCollider.height);
            component->setRadius(cylinderCollider.radius);
            return std::static_pointer_cast<T>(components[{entity, typeid(CylinderCollider)}]);
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

    template API_DLL std::shared_ptr<BoxCollider> Scene::addComponent<BoxCollider>(Entity);

    template API_DLL std::shared_ptr<BoxCollider> Scene::addComponentInternal<BoxCollider>(Entity);

    template API_DLL std::shared_ptr<SphereCollider> Scene::addComponent<SphereCollider>(Entity);

    template API_DLL std::shared_ptr<SphereCollider> Scene::addComponentInternal<SphereCollider>(Entity);

    template API_DLL std::shared_ptr<CapsuleCollider> Scene::addComponent<CapsuleCollider>(Entity);

    template API_DLL std::shared_ptr<CapsuleCollider> Scene::addComponentInternal<CapsuleCollider>(Entity);

    template API_DLL std::shared_ptr<CylinderCollider> Scene::addComponent<CylinderCollider>(Entity);

    template API_DLL std::shared_ptr<CylinderCollider> Scene::addComponentInternal<CylinderCollider>(Entity);

    template API_DLL std::shared_ptr<Tag> Scene::getComponent<Tag>(Entity entity);

    template API_DLL std::shared_ptr<Transform> Scene::getComponent<Transform>(Entity entity);

    template API_DLL std::shared_ptr<MeshRenderer> Scene::getComponent<MeshRenderer>(Entity entity);

    template API_DLL std::shared_ptr<BoxCollider> Scene::getComponent<BoxCollider>(Entity entity);

    template API_DLL std::shared_ptr<SphereCollider> Scene::getComponent<SphereCollider>(Entity entity);

    template API_DLL std::shared_ptr<CapsuleCollider> Scene::getComponent<CapsuleCollider>(Entity entity);

    template API_DLL std::shared_ptr<CylinderCollider> Scene::getComponent<CylinderCollider>(Entity entity);

    template API_DLL Tag& Scene::getComponentInternal<Tag>(Entity entity);

    template API_DLL Transform& Scene::getComponentInternal<Transform>(Entity entity);

    template API_DLL MeshRenderer& Scene::getComponentInternal<MeshRenderer>(Entity entity);

    template API_DLL BoxCollider& Scene::getComponentInternal<BoxCollider>(Entity entity);

    template API_DLL SphereCollider& Scene::getComponentInternal<SphereCollider>(Entity entity);

    template API_DLL CapsuleCollider& Scene::getComponentInternal<CapsuleCollider>(Entity entity);

    template API_DLL CylinderCollider& Scene::getComponentInternal<CylinderCollider>(Entity entity);
}
