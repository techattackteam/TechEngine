#include "SceneCamera.hpp"
#include "components/render/CameraComponent.hpp"

namespace TechEngine {
    SceneCamera::SceneCamera() : GameObject("SceneCamera") {
        addComponent<TransformComponent>();
        addComponent<CameraComponent>(false, CameraComponent::PERSPECTIVE);
    }
}
