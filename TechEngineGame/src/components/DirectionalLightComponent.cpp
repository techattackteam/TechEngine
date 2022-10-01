#include <glm/ext/matrix_transform.hpp>
#include "DirectionalLightComponent.hpp"
#include "../scene/Scene.hpp"

namespace TechEngine {

    DirectionalLightComponent::DirectionalLightComponent(GameObject *gameObject) : color(glm::vec4(1, 1, 1, 1)), Component(gameObject, "DirectionalLight") {

    }

    void DirectionalLightComponent::fixedUpdate() {
        viewMatrix = glm::lookAt(glm::vec3(0, 0, 0), getTransform().getOrientation(), glm::vec3(0, 1, 0));
    }

    glm::mat4 &DirectionalLightComponent::getProjectionMatrix() {
        return projectionMatrix;
    }

    glm::mat4 &DirectionalLightComponent::getViewMatrix() {
        return viewMatrix;
    }

    glm::vec4 &DirectionalLightComponent::getColor() {
        return color;
    }

}