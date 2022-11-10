#include "TransformComponent.hpp"
#include "core/Timer.hpp"

namespace TechEngine {
    TransformComponent::TransformComponent(TechEngine::GameObject *gameObject)
            : position(glm::vec3(0, 0, 0)), orientation(glm::vec3(0, 0, 0)), scale(glm::vec3(1, 1, 1)), model(glm::mat4(1.0f)), Component(gameObject, "TransformComponent") {
        lastPosition = position;
        lastOrientation = orientation;
    }

    glm::mat4 TransformComponent::getModelMatrix() {
        if (gameObject->getParent() == nullptr) {
            model = glm::translate(glm::mat4(1), glm::mix(lastPosition, position, TechEngineCore::Timer::getInstance().getInterpolation())) *
                    glm::toMat4(glm::quat(glm::radians(glm::mix(lastOrientation, orientation, TechEngineCore::Timer::getInstance().getInterpolation())))) *
                    glm::scale(glm::mat4(1), scale);
        } else {
            glm::vec3 position = this->position + gameObject->getParent()->getTransform().position;
            glm::vec3 lastPosition = this->lastPosition + gameObject->getParent()->getTransform().lastPosition;
            model = glm::translate(glm::mat4(1), glm::mix(lastPosition, position, TechEngineCore::Timer::getInstance().getInterpolation())) *
                    glm::toMat4(glm::quat(glm::radians(glm::mix(lastOrientation, orientation, TechEngineCore::Timer::getInstance().getInterpolation())))) *
                    glm::scale(glm::mat4(1), scale);

        }
        return model;
    }

    void TransformComponent::translate(glm::vec3 vector) {
        lastPosition = position;
        position += vector;
    }

    void TransformComponent::translateTo(glm::vec3 position) {
        lastPosition = position;
        this->position = position;
    }

    /*Rotate using Euler angles in Degrees*/
    void TransformComponent::rotate(glm::vec3 rotation) {
        lastOrientation = orientation;
        this->orientation = rotation;
    }

    /*Rotate using Quaternions in Degrees*/
    void TransformComponent::rotate(glm::quat quaternion) {
        rotate(glm::degrees(glm::eulerAngles(quaternion)));
    }

    void TransformComponent::setScale(glm::vec3 scale) {
        this->scale = scale;
    }

    void TransformComponent::lookAt(glm::vec3 lookPosition) {
        glm::mat4 lootAtMatrix = glm::lookAt(position, lookPosition, glm::vec3(0, 1, 0));
        rotate(glm::toQuat(lootAtMatrix));
    }

    glm::vec3 TransformComponent::getPosition() const {
        return position;
    }

    glm::vec3 TransformComponent::getOrientation() const {
        return orientation;
    }

    glm::vec3 TransformComponent::getScale() const {
        return scale;
    }
}

