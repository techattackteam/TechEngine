#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace TechEngine {
    class Scene;

    struct Camera {
        static constexpr std::string_view tag = "TechEngine::Camera";

        // Degrees, not radians.
        float verticalFieldOfView = 60.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        // EV100: the frame is scaled by 1 / (1.2 * 2^exposure), so 0 leaves it close to unscaled.
        float exposure = 0.0f;
        bool main = true;

        template<typename Archive>
        friend void visit(Archive& archive, Camera& value) {
            archive.field("verticalFieldOfView", value.verticalFieldOfView);
            archive.field("nearPlane", value.nearPlane);
            archive.field("farPlane", value.farPlane);
            archive.field("exposure", value.exposure);
            archive.field("main", value.main);
        }

        // Warns only while warnedNone is false, and clears it when a main camera is found, so the
        // caller keeps one flag per view and a scene that loses its camera again warns again.
        static bool findMainCamera(const Scene& scene, Entity& camera, bool& warnedNone);

        // False, changing nothing, when camera has no Camera. Fills cleared with every other entity
        // whose main it turned off, so an editor can record each one in the same edit.
        static bool setMainCamera(Scene& scene, Entity camera, std::vector<Entity>& cleared);

        static std::size_t countMainCameras(const Scene& scene);

    private:
        static bool findMainCameraBelow(const Scene& scene, Entity entity, Entity& camera);
    };

}
