#pragma once

#include "scene/Scene.hpp"
#include "components/CameraComponent.hpp"
#include "core/Core.hpp"

namespace TechEngine {
    class SceneHelper {
    public:
        Engine_API inline static CameraComponent *mainCamera;

        static void changeMainCameraTo(CameraComponent *cameraComponent);

        static bool findCameraComponent();

        static bool hasMainCamera();

        static void clear();
    };

}
