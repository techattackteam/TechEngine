#pragma once

#include <string>

#include "serialization/BufferStream.hpp"

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
        }

        void setGameObject(const GameObject& gameObject);

        virtual void Serialize(StreamWriter* stream);

        virtual void Deserialize(StreamReader* stream);

        TransformComponent& getTransform();

        GameObject* getGameObject();
    };
}
