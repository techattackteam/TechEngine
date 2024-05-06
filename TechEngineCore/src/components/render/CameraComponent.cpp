#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "CameraComponent.hpp"
#include "components/TransformComponent.hpp"
/*#include "renderer/RendererSettings.hpp"*/
#include "eventSystem/EventDispatcher.hpp"
#include "core/Logger.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    CameraComponent::CameraComponent(GameObject* gameObject, EventDispatcher& eventDispatcher) : Component(gameObject, eventDispatcher, "Camera") {
        mainCamera = false;
        this->projectionType = PERSPECTIVE;
        init();
    }

    CameraComponent::CameraComponent(GameObject* gameObject, EventDispatcher& eventDispatcher, bool mainCamera, ProjectionType projectionType) : Component(gameObject, eventDispatcher, "Camera") {
        this->mainCamera = mainCamera;
        this->projectionType = projectionType;
        init();
    }

    void CameraComponent::init() {
        /*EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](TechEngine::Event* event) {
            onWindowResizeEvent((WindowResizeEvent*)(event));
        });

        EventDispatcher::getInstance().subscribe(FramebufferResizeEvent::eventType, [this](Event* event) {
            onFramebufferResizeEvent((FramebufferResizeEvent*)(event));
        });*/

        updateViewMatrix();
        //updateProjectionMatrix();
    }

    void CameraComponent::update() {
        updateViewMatrix();
        //updateProjectionMatrix();
    }

    void CameraComponent::updateProjectionMatrix(float aspectRatio) {
        if (projectionType == PERSPECTIVE) {
            projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        } else if (projectionType == ORTHOGRAPHIC) {
            projectionMatrix = glm::ortho(-orthoSize * aspectRatio, orthoSize * aspectRatio, -orthoSize, orthoSize, nearPlane, farPlane);
        }
    }

    void CameraComponent::updateViewMatrix() {
        gameObject->getTransform().setScale(glm::vec3(1, 1, 1));
        glm::mat4 model = gameObject->getComponent<TransformComponent>()->getModelMatrix();
        viewMatrix = glm::inverse(model);
    }

    void CameraComponent::changeProjectionType(ProjectionType projectionType) {
        this->projectionType = projectionType;
    }

    glm::mat4 CameraComponent::getViewMatrix() {
        return viewMatrix;
    }

    glm::mat4 CameraComponent::getProjectionMatrix() {
        return projectionMatrix;
    }

    bool CameraComponent::isMainCamera() {
        return mainCamera;
    }

    void CameraComponent::setIsMainCamera(bool mainCamera) {
        this->mainCamera = mainCamera;
    }

    CameraComponent::ProjectionType& CameraComponent::getProjectionType() {
        return this->projectionType;
    }

    void CameraComponent::setFov(float fov) {
        this->fov = fov;
    }

    void CameraComponent::setNear(float nearPlane) {
        this->nearPlane = nearPlane;
    }

    void CameraComponent::setFar(float farPlane) {
        this->farPlane = farPlane;
    }

    float CameraComponent::getFov() {
        return fov;
    }

    float CameraComponent::getNear() {
        return nearPlane;
    }

    float CameraComponent::getFar() {
        return farPlane;
    }

    void CameraComponent::setOrthoSize(float orthoSize) {
        this->orthoSize = orthoSize;
    }

    float CameraComponent::getOrthoSize() {
        return orthoSize;
    }

    Component* CameraComponent::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        CameraComponent* cameraComponent = (CameraComponent*)componentToCopy;
        CameraComponent* newCameraComponent = new CameraComponent(gameObjectToAttach, eventDispatcher, cameraComponent->mainCamera, cameraComponent->projectionType);
        newCameraComponent->fov = cameraComponent->fov;
        newCameraComponent->nearPlane = cameraComponent->nearPlane;
        newCameraComponent->farPlane = cameraComponent->farPlane;
        newCameraComponent->orthoSize = cameraComponent->orthoSize;
        return newCameraComponent;
    }
}
