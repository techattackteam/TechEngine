#include "TransformComponent.hpp"
#include "components/TransformComponent.hpp"


namespace TechEngineAPI {
    TransformComponent::TransformComponent(TechEngine::TransformComponent* transformComponent) {
        this->transformComponent = transformComponent;
    }

    void TransformComponent::setPosition(glm::vec3 position) {
        this->transformComponent->translateTo(position);
    }

    void TransformComponent::setRotation(glm::vec3 rotation) {
        this->transformComponent->setRotation(rotation);
    }

    void TransformComponent::setScale(glm::vec3 scale) {
        this->transformComponent->setScale(scale);
    }

    void TransformComponent::translate(glm::vec3 translation) {
        this->transformComponent->translate(translation);
    }

    void TransformComponent::rotate(glm::vec3 rotation) {
        this->transformComponent->rotate(rotation);
    }

    void TransformComponent::lookAt(glm::vec3 position) {
        this->transformComponent->lookAt(position);
    }

    glm::vec3 TransformComponent::getPosition() const {
        return this->transformComponent->getPosition();
    }

    glm::vec3 TransformComponent::getRotation() const {
        return this->transformComponent->getOrientation();
    }

    glm::vec3 TransformComponent::getScale() const {
        return this->transformComponent->getScale();
    }
}
