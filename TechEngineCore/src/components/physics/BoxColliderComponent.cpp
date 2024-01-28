#include "BoxColliderComponent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    BoxColliderComponent::BoxColliderComponent(GameObject* gameObject, PhysicsEngine&physicsEngine) : Collider(gameObject, physicsEngine, "BoxColliderComponent") {
        size = glm::vec3(1, 1, 1);
        physicsEngine.addCollider(this);
    }

    BoxColliderComponent::BoxColliderComponent(GameObject* gameObject, PhysicsEngine&physicsEngine, glm::vec3 size, glm::vec3 offset) : Collider(gameObject, physicsEngine, "BoxColliderComponent"), size(size) {
        this->offset = offset;
    }

    void BoxColliderComponent::setSize(glm::vec3 size) {
        this->size = size;
        physicsEngine.addCollider(this);
    }

    glm::vec3 BoxColliderComponent::getSize() {
        return size;
    }
}
