#pragma once
#include "Panel.hpp"

#include"components/Components.hpp"
#include "UIUtils/Guizmo.hpp"

namespace TechEngine {
    class Tag;

    class SceneView : public Panel {
    private:
        SystemsRegistry& m_appSystemsRegistry;
        const std::vector<Entity>& m_selectedEntities;
        Transform cameraTransform;
        Camera sceneCamera;
        Guizmo guizmo;
        uint32_t frameBufferID;

        bool mouse2 = false;
        bool mouse3 = false;
        bool moving = false;
        int id;
        inline static int lastGuizmoID = -1;
        inline static int totalIds = 0;

    public:
        SceneView(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry, const std::vector<Entity>& selectedEntities);

        void onInit() override;

        void onUpdate() override;

        void processKeyPressed(Key) override;

        void processMouseDragging(glm::vec2 delta, unsigned long long mouseButtons) override;

        void processMouseScroll(float yOffset) override;

        void changeGuizmoOperation(ImGuizmo::OPERATION operation);

        void changeGuizmoMode(ImGuizmo::MODE mode);

        ImGuizmo::MODE getGuizmoMode() const;


        uint32_t getFrameBufferID() const {
            return frameBufferID;
        }

    private:
        void renderCameraFrustum();

        void renderColliders();

        void renderBox(Transform& transform, glm::vec3 center, glm::vec3 scale, glm::vec4 color) const;

        void renderSphere(Transform& transform, glm::vec3 center, float radius, glm::vec4 color) const;

        void renderCapsule(Transform& transform, glm::vec3 center, float radius, float height, glm::vec4 color) const;

        void renderCylinder(Transform& transform, glm::vec3 center, float radius, float height, glm::vec4 color) const;

        glm::vec4 getColor(const Tag& tag, bool collider) const;
    };
}
