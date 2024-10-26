#pragma once
#include "Panel.hpp"

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
        bool isWindowHovered = false;
        int id;
        inline static int lastUsingId = -1;
        inline static int totalIds = 0;

    public:
        SceneView(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry, const std::vector<Entity>& selectedEntities);

        void onInit() override;

        void onUpdate() override;

        void onKeyPressedEvent(Key& key) override;

        void onKeyReleasedEvent(Key& key) override;

        void onMouseScrollEvent(float xOffset, float yOffset) override;

        void onMouseMoveEvent(glm::vec2 delta) override;

        void changeGuizmoOperation(ImGuizmo::OPERATION operation);

        void changeGuizmoMode(ImGuizmo::MODE mode);

        void processShortcuts() override;

    private:
        void renderCameraFrustum();

        void renderColliders();

        void renderBox(Transform& transform, glm::vec3 center, glm::vec3 scale, glm::vec4 color);

        glm::vec4 getColor(const Tag& tag, bool collider) const;
    };
}
