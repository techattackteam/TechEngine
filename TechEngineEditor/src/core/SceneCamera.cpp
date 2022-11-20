#include "SceneCamera.hpp"
#include "components/CameraComponent.hpp"

namespace TechEngine {
    SceneCamera::SceneCamera() : GameObject("SceneCamera") {
        addComponent<CameraComponent>(true, CameraComponent::ORTHOGRAPHIC);
    }
}