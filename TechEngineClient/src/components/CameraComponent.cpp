#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "renderer/RendererSettings.hpp"
#include "event/EventDispatcher.hpp"
#include "core/Logger.hpp"
#include "scene/SceneHelper.hpp"

namespace TechEngine {
    CameraComponent::CameraComponent(GameObject *gameObject) : Component(gameObject, "Camera") {
        mainCamera = false;
        this->projectionType = PERSPECTIVE;
        init();
    }

    CameraComponent::CameraComponent(GameObject *gameObject, bool mainCamera, ProjectionType projectionType) : Component(gameObject, "Camera") {
        this->mainCamera = mainCamera;
        this->projectionType = projectionType;
        init();
    }

    void CameraComponent::init() {
        EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](TechEngine::Event *event) {
            onWindowResizeEvent((WindowResizeEvent *) (event));
        });

        EventDispatcher::getInstance().subscribe(FramebufferResizeEvent::eventType, [this](Event *event) {
            onFramebufferResizeEvent((FramebufferResizeEvent *) (event));
        });

        updateViewMatrix();
        updateProjectionMatrix();
    }

    void CameraComponent::update() {
        updateViewMatrix();
        updateProjectionMatrix();
    }

    void CameraComponent::updateProjectionMatrix() {
        if (projectionType == PERSPECTIVE) {
            projectionMatrix = glm::perspective(glm::radians(fov), RendererSettings::aspectRatio, nearPlane, farPlane);

        } else if (projectionType == ORTHOGRAPHIC) {
            projectionMatrix = glm::ortho(-orthoSize * RendererSettings::aspectRatio, orthoSize * RendererSettings::aspectRatio, -orthoSize, orthoSize, nearPlane, farPlane);
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

    void CameraComponent::onWindowResizeEvent(WindowResizeEvent *event) {
        updateProjectionMatrix();
    }

    void CameraComponent::onFramebufferResizeEvent(FramebufferResizeEvent *event) {
        if (RendererSettings::targetId == event->getId()) {
            updateProjectionMatrix();
        }
    }

    glm::mat4 TechEngine::CameraComponent::getViewMatrix() {
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
        SceneHelper::changeMainCameraTo(this);
    }

    CameraComponent::ProjectionType &CameraComponent::getProjectionType() {
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
}
