#include "MainCamera.hpp"
#include "components/render/CameraComponent.hpp"

namespace TechEngine {
    MainCamera::MainCamera() : GameObject("MainCamera") {
        addComponent<CameraComponent>(true, CameraComponent::PERSPECTIVE);
    }
}
