#include "Transform.hpp"

namespace TechEngine {
    Transform::Transform(TechEngine::GameObject *gameObject)
            : position(glm::vec3(0, 0, 0)), orientation(glm::vec3(0, 0, 0)), scale(glm::vec3(1, 1, 1)), model(glm::mat4(1.0f)), Component(gameObject, "Transform") {
    }

    glm::mat4 Transform::getModelMatrix() {
        model = glm::translate(glm::mat4(1), position) * glm::toMat4(glm::quat(glm::radians(orientation))) * glm::scale(glm::mat4(1), scale);
        return model;
    }

    void Transform::translate(glm::vec3 vector) {
        position += vector;
    }

    void Transform::translateTo(glm::vec3 position) {
        translate(position - this->position);
        this->position = position;
    }

    /*Rotate using Euler angles in Degrees*/
    void Transform::rotate(glm::vec3 rotation) {
        this->orientation = rotation;
    }

    /*Rotate using Quaternions in Degrees*/
    void Transform::rotate(glm::quat quaternion) {
        rotate(glm::degrees(glm::eulerAngles(quaternion)));
    }

    void Transform::setScale(glm::vec3 scale) {
        this->scale = scale;
    }

    void Transform::lookAt(glm::vec3 lookPosition) {
        glm::mat4 lootAtMatrix = glm::lookAt(position, lookPosition, glm::vec3(0, 1, 0));
        rotate(glm::toQuat(lootAtMatrix));
    }

    glm::vec3 Transform::getPosition() const {
        return position;
    }

    glm::vec3 Transform::getOrientation() const {
        return orientation;
    }

    void Transform::getInfo() {

    }
}

