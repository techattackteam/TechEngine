#pragma once
#include "Panel.hpp"
#include "components/Archetype.hpp"
#include "systems/SystemsRegistry.hpp"
#include "UIUtils/Guizmo.hpp"

namespace TechEngine {
    class Tag;

    class SceneView : public Panel {
    private:
        SystemsRegistry& m_appSystemsRegistry;
        const std::vector<Entity>& m_selectedEntities;
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
        SceneView(SystemsRegistry& appSystemsRegistry, const std::vector<Entity>& selectedEntities);

        void onInit() override;

        void onUpdate() override;

        void onKeyPressedEvent(Key& key);

        void onKeyReleasedEvent(Key& key);

        void onMouseScrollEvent(float xOffset, float yOffset);

        void onMouseMoveEvent(glm::vec2 delta);

        void processShortcuts();
    };
}
