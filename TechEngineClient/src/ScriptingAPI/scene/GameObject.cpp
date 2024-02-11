#include "GameObject.hpp"

#include "scene/GameObject.hpp"
#include "ScriptingAPI/components/TransformComponent.hpp"

namespace TechEngineAPI {
    std::shared_ptr<TechEngineAPI::TransformComponent> GameObject::getTransform() const {
        return std::make_shared<TechEngineAPI::TransformComponent>(&gameObject->getTransform());
    }

    std::string GameObject::getName() const {
        return gameObject->getName();
    }
}
