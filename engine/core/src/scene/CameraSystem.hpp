#pragma once
#include "Scene.hpp"
#include "components/Components.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class CORE_DLL CameraSystem : public System {
    private:
        SystemsRegistry& m_systemsRegistry;

    public:
        CameraSystem(SystemsRegistry& systemsRegistry);

        void setFov(Entity entity, float fov);

        void setNear(Entity entity, float nearPlane);

        void setFar(Entity entity, float common_far);

        void setMainCamera(Entity entity);

        bool hasMainCamera();

        Camera& getMainCamera();

        void removeMainCamera();
    };
}
