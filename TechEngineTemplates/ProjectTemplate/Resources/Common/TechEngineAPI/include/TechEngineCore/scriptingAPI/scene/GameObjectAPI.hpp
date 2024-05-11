#pragma once
#include <string>

#include "components/TransformComponent.hpp"

namespace TechEngine {
    class GameObject;
}

namespace TechEngine {
    class GameObjectAPI {
    private:
        GameObject* gameObject;
        friend class SceneAPI;

    public:
        explicit GameObjectAPI(GameObject* gameObject) : gameObject(gameObject) {
        }

        TransformComponent* getTransform() const;

        template<class T>
        T* getComponent();

        template<class C, typename... A>
        void addComponent(A&&... args);

        std::string getName() const;

        std::string getTag() const;
    };
}
