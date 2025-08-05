#pragma once
#include "Panel.hpp"

#include <RmlUi/Core/Context.h>

#include "UIUtils/Guizmo.hpp"


namespace TechEngine {
    class UIEditor;

    class UIView : public Panel {
    private:
        UIEditor* m_uiEditor = nullptr;
        SystemsRegistry& m_appSystemsRegistry;
        Transform cameraTransform;
        Camera sceneCamera;

        uint32_t m_frameBufferID = 0;
        Rml::Context* m_context = nullptr;
        ImVec2 m_lastMousePosition;
        Guizmo guizmo;

        bool mouse2 = false;
        bool mouse3 = false;
        bool moving = false;
        bool isWindowHovered = false;
        int id = 1000; // Unique ID for the view, can be used for guizmo operations
        inline static int lastUsingId = -1;
        inline static int totalIds = 0;

    public:
        UIView(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry, UIEditor* uiEditor);

        void onInit() override;

        void onUpdate() override;

        void onKeyPressedEvent(Key& key) override;

        void onKeyReleasedEvent(Key& key) override;

        void onMouseScrollEvent(float xOffset, float yOffset) override;

        void onMouseMoveEvent(glm::vec2 delta) override;

    private:
        void drawHelperLines(ImVec2 imageTopLeft);

        ImVec2 convertRmlToImGui(const Rml::Vector2f& rmlPos, const ImVec2& imageTopLeft, const ImVec2& imageSize);
    };
}
