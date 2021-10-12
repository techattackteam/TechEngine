#include <glm/ext/matrix_transform.hpp>
#include "DirectionalLightComponent.hpp"
#include "../scene/Scene.hpp"

namespace Engine {

    DirectionalLightComponent::DirectionalLightComponent(GameObject *gameObject) : gameObject(*gameObject), Component("DirectionalLight") {

    }

    void DirectionalLightComponent::fixedUpdate() {
        viewMatrix = glm::lookAt(Scene::getInstance().mainCamera->getTransform().getPosition(), gameObject.getTransform().getOrientation(), glm::vec3(0, 0, 0));
    }

    glm::mat4 &DirectionalLightComponent::getProjectionMatrix() {
        return projectionMatrix;
    }

    glm::mat4 &DirectionalLightComponent::getViewMatrix() {
        return viewMatrix;
    }

    void DirectionalLightComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {

        }
    }
}