#pragma once
#include <memory>
#include <string>


namespace TechEngine {
    class GameObject;
}

namespace TechEngineAPI {
    class TransformComponent;
    class MeshRendererComponent;

    class GameObject {
    private:
        TechEngine::GameObject* gameObject;

    public:
        GameObject(TechEngine::GameObject* gameObject) : gameObject(gameObject) {
        }

        void addMeshRendererComponent();

        std::shared_ptr<TechEngineAPI::TransformComponent> getTransform() const;

        std::shared_ptr<TechEngineAPI::MeshRendererComponent> getMeshRenderer() const;

        std::string getName() const;
    };
}
