#pragma once

#include <string>

#include "eventSystem/EventDispatcher.hpp"
#include "serialization/BufferStream.hpp"

namespace TechEngine {
    class GameObject;
    class TransformComponent;

    class Component {
    protected:
        std::string name;
        GameObject* gameObject;
        EventDispatcher& eventDispatcher;

    public:
        Component(GameObject* gameObject, EventDispatcher& eventDispatcher, std::string name);

        virtual ~Component();

        virtual void fixedUpdate();

        virtual void update();

        virtual Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) = 0;

        virtual std::string& getName() {
            return name;
        }

        virtual void Serialize(StreamWriter* stream);

        virtual void Deserialize(StreamReader* stream);

        TransformComponent& getTransform();

        GameObject* getGameObject();
    };
}
