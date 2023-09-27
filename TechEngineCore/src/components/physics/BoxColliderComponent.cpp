#include "BoxColliderComponent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    BoxColliderComponent::BoxColliderComponent(GameObject *gameObject) : Collider(gameObject, "BoxColliderComponent") {
        size = glm::vec3(1, 1, 1);
        PhysicsEngine::getInstance()->addCollider(this);
    }

    BoxColliderComponent::BoxColliderComponent(GameObject *gameObject, glm::vec3 size, glm::vec3 offset) :
            Collider(gameObject, "BoxColliderComponent"), size(size) {
        this->offset = offset;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    BoxColliderComponent::~BoxColliderComponent() {
        PhysicsEngine::getInstance()->removeCollider(gameObject->getTag(), this);
    }

    void BoxColliderComponent::setSize(glm::vec3 size) {
        this->size = size;
        PhysicsEngine::getInstance()->addCollider(this);
    }

    glm::vec3 BoxColliderComponent::getSize() {
        return size;
    }
}
