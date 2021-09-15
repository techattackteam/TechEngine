#pragma once

#include "Component.hpp"
#include "../scene/GameObject.hpp"
#include <glm/glm.hpp>

namespace Engine {
    class CameraComponent : public Component {
    private:

        GameObject *gameObject;

        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;

        float fov;
        float nearPlane;
        float farPlane;

        bool mainCamera = false;

    public:
        explicit CameraComponent(GameObject *gameObject);

        CameraComponent(GameObject *gameObject, bool mainCamera);

        void updateProjectionMatrix();

        void updateViewMatrix();

        glm::mat4 getViewMatrix();

        glm::mat4 getProjectionMatrix();

        void getInfo() override;

        bool isMainCamera();

    };
}


