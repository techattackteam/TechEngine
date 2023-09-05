#include "SceneCamera.hpp"
#include "components/CameraComponent.hpp"

namespace TechEngine {
    SceneCamera::SceneCamera() {
        addComponent<TransformComponent>();
        addComponent<CameraComponent>(false, CameraComponent::PERSPECTIVE);
    }
}