#pragma once

#include "Panel.hpp"
#include "renderer/Renderer.hpp"
#include "UIUtils/Guizmo.hpp"
#include "core/SceneCamera.hpp"
#include "components/CameraComponent.hpp"

namespace TechEngine {
    class SceneView : public Panel {
        uint32_t frameBufferID;
        Renderer *renderer;
        SceneCamera *sceneCamera;
        Guizmo guizmo;
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
        void renderBoxCollider(GameObject *gameObject);

        void renderSphereCollider(GameObject *gameObject);

        void renderCylinderCollier(GameObject *gameObject);
    };
}


