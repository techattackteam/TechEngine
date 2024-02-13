#include "GameObject.hpp"

#include "scene/GameObject.hpp"

#include "components/MeshRendererComponent.hpp"

#include "ScriptingAPI/components/TransformComponent.hpp"
#include "ScriptingAPI/components/MeshRendererComponent.hpp"

namespace TechEngineAPI {
    void GameObject::addMeshRendererComponent() {
        gameObject->addComponent<TechEngine::MeshRendererComponent>();
    }


    std::shared_ptr<TechEngineAPI::TransformComponent> GameObject::getTransform() const {
        return std::make_shared<TechEngineAPI::TransformComponent>(&gameObject->getTransform());
    }

    std::shared_ptr<TechEngineAPI::MeshRendererComponent> GameObject::getMeshRenderer() const {
        return std::make_shared<TechEngineAPI::MeshRendererComponent>(gameObject->getComponent<TechEngine::MeshRendererComponent>());
    }


    std::string GameObject::getName() const {
        return gameObject->getName();
    }
}
