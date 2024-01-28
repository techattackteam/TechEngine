#pragma once

#include "scene/Scene.hpp"
#include "components/CameraComponent.hpp"

namespace TechEngine {
    class SceneHelper {
    public:
         inline static CameraComponent *mainCamera;

        static void changeMainCameraTo(CameraComponent *cameraComponent);

        static bool findCameraComponent(Scene& scene);

        static bool hasMainCamera(Scene& scene);

        static void clear(Scene& scene);
    };

}
