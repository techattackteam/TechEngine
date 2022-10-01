#pragma once

#include "components/Component.hpp"
#include "../scene/GameObject.hpp"
#include "components/TransformComponent.hpp"
#include "../events/window/WindowResizeEvent.hpp"
#include <glm/glm.hpp>

namespace TechEngine {

    class CameraComponent : public Component {
    public:
        enum ProjectionType {
            PERSPECTIVE, ORTHOGRAPHIC
        };
    private:
        ProjectionType projectionType = PERSPECTIVE;

        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);

        float fov = 90;
        float nearPlane;
        float farPlane;

        bool mainCamera = false;

    public:
        explicit CameraComponent(GameObject *gameObject);

        CameraComponent(GameObject *gameObject, bool mainCamera, ProjectionType projectionType);

        void init();

        void fixedUpdate() override;

        void updateProjectionMatrix();

        void updateViewMatrix();

        void changeProjectionType(ProjectionType projectionType);

        void onWindowResizeEvent(WindowResizeEvent *event);

        glm::mat4 getViewMatrix();

        glm::mat4 getProjectionMatrix();

        bool isMainCamera();
    };
}


