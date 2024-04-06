#pragma once

#include <string>

namespace TechEngine {
    class GameObject;
    class TransformComponent;

    class Component {
    protected:
        std::string name;
        GameObject* gameObject;

    public:
        Component(GameObject* gameObject, std::string name);

        virtual ~Component();

        virtual void fixedUpdate();

        virtual void update();

        virtual Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) = 0;

        virtual std::string& getName() {
            return name;
        };

        TransformComponent& getTransform();

        GameObject* getGameObject();
    };
}
