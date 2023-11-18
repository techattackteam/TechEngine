#pragma once

#include "Panel.hpp"
#include "renderer/Renderer.hpp"
#include "UIUtils/Guizmo.hpp"
#include "core/SceneCamera.hpp"
#include "components/CameraComponent.hpp"
#include "core/Key.hpp"

namespace TechEngine {
    class SceneView : public Panel {
        uint32_t frameBufferID;
        Renderer *renderer;
        SceneCamera *sceneCamera;
        Guizmo guizmo;

        bool mouse2 = false;
        bool mouse3 = false;
        bool moving = false;
        bool isWindowHovered = false;
    public:
        SceneView(Renderer &renderer);

        void onUpdate() override;

        void changeGuizmoOperation(int operation);

        void changeGuizmoMode(int mode);

        int getGuizmoMode() const;

        SceneCamera *getSceneCamera() const {
            return sceneCamera;
        }

        void renderCameraFrustum(TechEngine::CameraComponent *camera);

        void renderColliders();

    private:
        glm::vec4 getColor(GameObject *gameObject);

        void renderBoxCollider(GameObject *gameObject);

        void renderSphereCollider(GameObject *gameObject);

        void renderCylinderCollier(GameObject *gameObject);

        void OnKeyPressedEvent(Key &key);

        void OnKeyReleasedEvent(Key &key);

        void OnMouseScrollEvent(float xOffset, float yOffset);

        void OnMouseMoveEvent(glm::vec2 delta);

    };
}


