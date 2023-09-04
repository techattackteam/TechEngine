#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "renderer/RendererSettings.hpp"
#include "event/EventDispatcher.hpp"

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
        TechEngine::EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](TechEngine::Event *event) {
            onWindowResizeEvent((WindowResizeEvent *) (event));
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
            projectionMatrix = glm::perspective(glm::radians(fov), RendererSettings::aspectRatio, 0.1f, 50.0f);
        } else if (projectionType == ORTHOGRAPHIC) {
            projectionMatrix = glm::ortho(-5.0f * RendererSettings::aspectRatio, 5.0f * RendererSettings::aspectRatio, -5.0f, 5.0f, 0.3f, 1000.0f);
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
    }

    CameraComponent::ProjectionType &CameraComponent::getProjectionType() {
        return this->projectionType;
    }
}
