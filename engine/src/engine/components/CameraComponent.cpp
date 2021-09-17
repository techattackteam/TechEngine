#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "CameraComponent.hpp"
#include "TransformComponent.hpp"


namespace Engine {
    CameraComponent::CameraComponent(GameObject *gameObject) : Component("Camera") {
        this->gameObject = gameObject;
        mainCamera = false;
    }

    CameraComponent::CameraComponent(GameObject *gameObject, bool mainCamera) : Component("Camera") {
        this->gameObject = gameObject;
        this->mainCamera = mainCamera;
    }

    void CameraComponent::updateProjectionMatrix() {
        projectionMatrix = glm::perspective(glm::radians(fov), 16.0f / 9.0f, nearPlane, farPlane);
    }

    void CameraComponent::updateViewMatrix() {
        glm::lookAt(((TransformComponent *) gameObject->getComponent<TransformComponent>())->getPosition(), glm::vec3(0, 1, 0), glm::vec3(0, 0, 0));
    }

    glm::mat4 Engine::CameraComponent::getViewMatrix() {
        return viewMatrix;
    }

    glm::mat4 CameraComponent::getProjectionMatrix() {
        return projectionMatrix;
    }

    void CameraComponent::getInfo() {

    }

    bool CameraComponent::isMainCamera() {
        return mainCamera;
    }


}
