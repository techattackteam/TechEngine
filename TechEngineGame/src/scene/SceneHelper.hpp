#pragma once

#include "scene/Scene.hpp"
#include "components/CameraComponent.hpp"
#include "core/Core.hpp"

namespace TechEngine {
    class SceneHelper {
    public:
        inline static CameraComponent *mainCamera = nullptr;

        static bool findCameraComponent();

        static bool hasMainCamera();
    };
}
