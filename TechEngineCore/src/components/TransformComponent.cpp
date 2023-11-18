#include "TransformComponent.hpp"
#include "core/Timer.hpp"

namespace TechEngine {
    TransformComponent::TransformComponent(TechEngine::GameObject *gameObject)
            : position(glm::vec3(0, 0, 0)), orientation(glm::vec3(0, 0, 0)), scale(glm::vec3(1, 1, 1)), model(glm::mat4(1.0f)), Component(gameObject, "TransformComponent") {
        lastPosition = position;
        lastOrientation = orientation;
    }

    glm::mat4 TransformComponent::getModelMatrix() {
        GameObject *parent = gameObject->getParent();
        glm::vec3 position = glm::vec3(0, 0, 0);
        glm::vec3 lastPosition = position;
        glm::vec3 orientation = glm::vec3(0, 0, 0);
        glm::vec3 scale = glm::vec3(1, 1, 1);
        model = glm::mat4(1.0f);
        while (parent != nullptr) {
            position = parent->getTransform().position;
            orientation = parent->getTransform().orientation;
            scale = parent->getTransform().scale;
            model *= glm::translate(glm::mat4(1), position) * glm::toMat4(glm::quat(glm::radians(orientation))) * glm::scale(glm::mat4(1), scale);
            parent = parent->getParent();
        }
        model *= glm::translate(glm::mat4(1), this->position) * glm::toMat4(glm::quat(glm::radians(this->orientation))) * glm::scale(glm::mat4(1), this->scale);

        return model;
    }

    glm::mat4 TransformComponent::getLocalModelMatrix() {
        return glm::translate(glm::mat4(1), position) * glm::toMat4(glm::quat(glm::radians(orientation))) * glm::scale(glm::mat4(1), scale);
    }

    void TransformComponent::translate(glm::vec3 vector) {
        lastPosition = position;
        position += vector;
    }

    void TransformComponent::translateTo(glm::vec3 position) {
        lastPosition = position;
        this->position = position;
    }

    void TransformComponent::translateToWorld(glm::vec3 worldPosition) {
        GameObject *parent = gameObject->getParent();
        glm::vec3 position = glm::vec3(0, 0, 0);
        while (parent != nullptr) {
            position += parent->getTransform().position;

            parent = parent->getParent();

        }
        translateTo(worldPosition - position);
    }

    /*Rotate using Euler angles in Degrees*/
    void TransformComponent::setRotation(glm::vec3 rotation) {
        lastOrientation = rotation;
        this->orientation = rotation;
    }

    /*Rotate using Quaternions in Degrees*/
    void TransformComponent::setRotation(glm::quat quaternion) {
        setRotation(glm::degrees(glm::eulerAngles(quaternion)));
    }

    void TransformComponent::rotate(glm::vec3 rotation) {
        lastOrientation = orientation;
        this->orientation += rotation;
    }

    void TransformComponent::setScale(glm::vec3 scale) {
        this->scale = scale;
    }

    void TransformComponent::lookAt(glm::vec3 lookPosition) {
        glm::mat4 lootAtMatrix = glm::lookAt(position, lookPosition, glm::vec3(0, 1, 0));
        setRotation(glm::toQuat(lootAtMatrix));
    }

    glm::vec3 TransformComponent::getPosition() const {
        return position;
    }

    glm::vec3 TransformComponent::getWorldPosition() const {
        GameObject *parent = gameObject->getParent();
        glm::vec3 position = this->position;
        while (parent != nullptr) {
            position += parent->getTransform().position;

            parent = parent->getParent();
        }
        return position;
    }

    glm::vec3 TransformComponent::getOrientation() const {
        return orientation;
    }

    glm::vec3 TransformComponent::getScale() const {
        return scale;
    }

}

