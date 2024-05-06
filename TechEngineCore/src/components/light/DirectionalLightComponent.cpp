#include <glm/ext/matrix_transform.hpp>
#include "DirectionalLightComponent.hpp"
#include "components/TransformComponent.hpp"

namespace TechEngine {
    DirectionalLightComponent::DirectionalLightComponent(GameObject* gameObject, EventDispatcher& eventDispatcher) : color(glm::vec4(1, 1, 1, 1)),
                                                                                                                     Component(gameObject, eventDispatcher, "DirectionalLight") {
    }

    void DirectionalLightComponent::fixedUpdate() {
        viewMatrix = glm::lookAt(glm::vec3(0, 0, 0), getTransform().getOrientation(), glm::vec3(0, 1, 0));
    }

    glm::mat4& DirectionalLightComponent::getProjectionMatrix() {
        return projectionMatrix;
    }

    glm::mat4& DirectionalLightComponent::getViewMatrix() {
        return viewMatrix;
    }

    glm::vec4& DirectionalLightComponent::getColor() {
        return color;
    }

    Component* DirectionalLightComponent::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        auto* component = new DirectionalLightComponent(gameObjectToAttach, eventDispatcher);
        component->color = color;
        component->viewMatrix = viewMatrix;
        component->projectionMatrix = projectionMatrix;
        return component;
    }
}
