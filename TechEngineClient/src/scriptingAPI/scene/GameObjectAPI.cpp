#include "GameObjectAPI.hpp"

#include "components/CameraComponent.hpp"
#include "components/MeshRendererComponent.hpp"
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

    template MeshRendererComponent* GameObjectAPI::getComponent<MeshRendererComponent>();

    template void GameObjectAPI::addComponent<MeshRendererComponent>();

    template void GameObjectAPI::addComponent<TransformComponent>();
}
