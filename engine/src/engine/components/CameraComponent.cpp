#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include "CameraComponent.hpp"
#include "TransformComponent.hpp"

namespace Engine {
    CameraComponent::CameraComponent(GameObject *gameObject) : Component("Camera") {
        this->gameObject = gameObject;
        mainCamera = false;

        updateProjectionMatrix();
        updateViewMatrix();
    }

    CameraComponent::CameraComponent(GameObject *gameObject, bool mainCamera) : Component("Camera") {
        this->gameObject = gameObject;
        this->mainCamera = mainCamera;

    }

    void CameraComponent::update() {
        updateViewMatrix();
        updateProjectionMatrix();
    }

    void CameraComponent::updateProjectionMatrix() {
        projectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    }

    void CameraComponent::updateViewMatrix() {
        glm::mat4 model = gameObject->getComponent<TransformComponent>()->getModelMatrix();
        model[0][0] = 1;
        model[1][1] = 1;
        model[2][2] = 1;
        model[3][3] = 1;
        viewMatrix = glm::inverse(model);
    }

    glm::mat4 Engine::CameraComponent::getViewMatrix() {
        return viewMatrix;
    }

    glm::mat4 CameraComponent::getProjectionMatrix() {
        return projectionMatrix;
    }

    void CameraComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {

        }

    }

    bool CameraComponent::isMainCamera() {
        return mainCamera;
    }


}
