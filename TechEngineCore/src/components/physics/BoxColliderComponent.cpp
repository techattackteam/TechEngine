#include "BoxColliderComponent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    BoxColliderComponent::BoxColliderComponent(GameObject *gameObject) : Collider(gameObject, "BoxColliderComponent") {
        size = glm::vec3(1, 1, 1);
        offset = glm::vec3(0, 0, 0);
        PhysicsEngine::getInstance()->addCollider(this);
    }


    BoxColliderComponent::BoxColliderComponent(GameObject *gameObject, glm::vec3 size, glm::vec3 offset, bool dynamic) :
            Collider(gameObject, "BoxColliderComponent"), size(size), offset(offset) {
        PhysicsEngine::getInstance()->addCollider(this);
    }

    void BoxColliderComponent::setSize(glm::vec3 size) {
        this->size = size;
    }

    glm::vec3 BoxColliderComponent::getSize() {
        return size;
    }

    void BoxColliderComponent::setOffset(glm::vec3 offset) {
        this->offset = offset;
    }

    glm::vec3 BoxColliderComponent::getOffset() {
        return offset;
    }
}
