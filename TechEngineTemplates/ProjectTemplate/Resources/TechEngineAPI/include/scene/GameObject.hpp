#pragma once
#include <memory>
#include <string>


namespace TechEngine {
    class GameObject;
}

namespace TechEngineAPI {
    class TransformComponent;

    class GameObject {
    private:
        TechEngine::GameObject* gameObject;

    public:
        GameObject(TechEngine::GameObject* gameObject) : gameObject(gameObject) {
        }

        template<typename C>
        std::shared_ptr<C> getComponent() {
            C* component = gameObject->template getComponent<C>();
            return std::make_shared<C>(component);
        }

        std::shared_ptr<TechEngineAPI::TransformComponent> getTransform() const;

        std::string getName() const;
    };
}
