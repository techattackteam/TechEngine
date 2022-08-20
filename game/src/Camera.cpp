#include "Camera.hpp"

Camera::Camera() : TechEngine::GameObject("Camera", false) {
    addComponent<TechEngine::CameraComponent>(this, true, TechEngine::CameraComponent::ORTHOGRAPHIC);
    getTransform().translateTo(glm::vec3(0, 0, 5));
}
