#pragma once

#include "scene/GameObject.hpp"
#include "components/TransformComponent.hpp"
#include <glm/glm.hpp>

namespace TechEngine {
    class CORE_DLL CameraComponent : public Component {
    public:
        enum ProjectionType {
            PERSPECTIVE, ORTHOGRAPHIC
        };

    private:
        ProjectionType projectionType = PERSPECTIVE;

        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);

        float fov = 90;
        float nearPlane = 0.1f;
        float farPlane = 100;
        float orthoSize = 5;

    public:
        bool mainCamera = false;

        explicit CameraComponent(GameObject* gameObject, SystemsRegistry& systemsRegistry);

        CameraComponent(GameObject* gameObject, SystemsRegistry& systemsRegistry, bool mainCamera, ProjectionType projectionType);

        virtual void init();

        void update() override;

        void updateProjectionMatrix(float aspectRatio);

        void updateViewMatrix();

        void changeProjectionType(ProjectionType projectionType);

        glm::mat4 getViewMatrix();

        glm::mat4 getProjectionMatrix();

        bool isMainCamera();

        void setIsMainCamera(bool mainCamera);

        ProjectionType& getProjectionType();

        float getNear();

        float getFar();

        float getFov();

        float getOrthoSize();

        void setFar(float farPlane);

        void setNear(float nearPlane);

        void setFov(float fov);

        void setOrthoSize(float orthoSize);

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
