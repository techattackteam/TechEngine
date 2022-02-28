#include "Camera.hpp"

Camera::Camera() : Engine::GameObject("Camera", false) {
    addComponent<Engine::CameraComponent>(this, true);
    getTransform().translateTo(glm::vec3(0, 0, 5));
}
