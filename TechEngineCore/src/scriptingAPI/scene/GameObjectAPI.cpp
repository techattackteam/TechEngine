#include "GameObjectAPI.hpp"


#include "components/network/NetworkSync.hpp"
#include "components/render/CameraComponent.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "components/physics/BoxColliderComponent.hpp"
#include "components/physics/SphereCollider.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "components/physics/RigidBody.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    TransformComponent* GameObjectAPI::getTransform() const {
        return gameObject->getComponent<TransformComponent>();
    }

    template<class T>
    T* GameObjectAPI::getComponent() {
        return (T*)gameObject->template getComponent<T>();
    }

    template<class C, typename... A>
    void GameObjectAPI::addComponent(A&&... args) {
        gameObject->template addComponent<C>(std::forward<A>(args)...);
    }

    std::string GameObjectAPI::getName() const {
        return gameObject->getName();
    }

    template TransformComponent* GameObjectAPI::getComponent<TransformComponent>();

    template CameraComponent* GameObjectAPI::getComponent<CameraComponent>();

    template MeshRendererComponent* GameObjectAPI::getComponent<MeshRendererComponent>();

    template BoxColliderComponent* GameObjectAPI::getComponent<BoxColliderComponent>();

    template SphereCollider* GameObjectAPI::getComponent<SphereCollider>();

    template CylinderCollider* GameObjectAPI::getComponent<CylinderCollider>();

    template RigidBody* GameObjectAPI::getComponent<RigidBody>();

    template NetworkSync* GameObjectAPI::getComponent<NetworkSync>();

    template void GameObjectAPI::addComponent<TransformComponent>();

    template void GameObjectAPI::addComponent<CameraComponent>();

    template void GameObjectAPI::addComponent<MeshRendererComponent>();

    template void GameObjectAPI::addComponent<BoxColliderComponent>();

    template void GameObjectAPI::addComponent<SphereCollider>();

    template void GameObjectAPI::addComponent<CylinderCollider>();

    template void GameObjectAPI::addComponent<RigidBody>();

    template void GameObjectAPI::addComponent<NetworkSync>();
}
