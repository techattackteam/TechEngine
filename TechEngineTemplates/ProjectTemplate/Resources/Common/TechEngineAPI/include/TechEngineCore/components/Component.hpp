#pragma once

#include <string>

#include "serialization/BufferStream.hpp"

namespace TechEngine {
    class GameObject;
    class TransformComponent;
    class SystemsRegistry;

    class CORE_DLL Component {
    protected:
        std::string name;
        GameObject* gameObject;
        SystemsRegistry& systemsRegistry;

    public:
        Component(GameObject* gameObject, SystemsRegistry& systemsRegistry, std::string name);

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
