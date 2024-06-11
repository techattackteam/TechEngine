#pragma once

#include <glm/glm.hpp>
#include "Component.hpp"
#include "serialization/BufferStream.hpp"

namespace TechEngine {
    class TransformComponent : public Component {
    public:
        glm::vec3 lastPosition;
        glm::vec3 lastOrientation;

        glm::vec3 position;
        glm::vec3 orientation;
        glm::vec3 scale;
        glm::mat4 model;
        glm::vec3 forward;

        explicit TransformComponent(GameObject* gameObject, EventDispatcher& eventDispatcher);

        void update() override;

        void translate(glm::vec3 vector);

        void translateTo(glm::vec3 position);

        void translateToWorld(glm::vec3 worldPosition);

        void setRotation(glm::vec3 rotation);

        void rotate(glm::vec3 rotation);

        void setRotation(glm::quat quaternion);

        void setScale(glm::vec3 vector);

        void lookAt(glm::vec3 position);

        glm::mat4 getModelMatrix();

        glm::mat4 getLocalModelMatrix();

        glm::vec3 getPosition() const;

        glm::vec3 getWorldPosition() const;

        glm::vec3 getOrientation() const;

        glm::vec3 getScale() const;

        glm::vec3 getForward() const;

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;

        virtual void Serialize(StreamWriter* stream);

        virtual void Deserialize(StreamReader* stream);
    };
}
