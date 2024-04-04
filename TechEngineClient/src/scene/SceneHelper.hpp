#pragma once

#include "components/CameraComponent.hpp"
#include "core/Core.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    class TECHENGINE_API SceneHelper {
    public:
        inline static CameraComponent* mainCamera;

        static void changeMainCameraTo(CameraComponent* cameraComponent);

        static bool findCameraComponent(Scene& scene);

        static bool hasMainCamera(Scene& scene);

        static void clear(Scene& scene);
    };
}
