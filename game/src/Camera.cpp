#include <engine/components/TransformComponent.hpp>
#include "Camera.hpp"

Camera::Camera() : Engine::GameObject("Camera", false) {
    addComponent<Engine::CameraComponent>(this, true);
    getComponent<Engine::TransformComponent>()->translateTo(glm::vec3(0, 0, 5));
   // getComponent<Engine::TransformComponent>()->lookAt(glm::vec3(0, 0, 0));
}
