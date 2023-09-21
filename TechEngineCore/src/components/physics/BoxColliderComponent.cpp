#include "BoxColliderComponent.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    BoxColliderComponent::BoxColliderComponent(GameObject *gameObject) : Collider(gameObject, "BoxColliderComponent") {
        size = glm::vec3(1, 1, 1);
    }


    BoxColliderComponent::BoxColliderComponent(GameObject *gameObject, glm::vec3 size, glm::vec3 offset, bool dynamic) :
            Collider(gameObject, "BoxColliderComponent", dynamic), size(size) {
        this->offset = offset;
    }

    BoxColliderComponent::~BoxColliderComponent() {
        PhysicsEngine::getInstance()->removeCollider(gameObject->getTag(), this);
    }

    void BoxColliderComponent::setSize(glm::vec3 size) {
        this->size = size;
    }

    glm::vec3 BoxColliderComponent::getSize() {
        return size;
    }
}
