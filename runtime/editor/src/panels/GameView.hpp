#pragma once
#include "Panel.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    class GameView : public Panel {
    private:
        SystemsRegistry& m_appSystemsRegistry;
        uint32_t frameBufferID;

    public:
        GameView(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        glm::vec2 getFrameBufferSize();

        void processMouseMoving(glm::vec2 delta) override;

        void processMouseScroll(float yOffset) override;

        void processMouseDragging(glm::vec2 delta, unsigned long long mouseButtons) override;

        void processKeyPressed(Key key) override;

        void processKeyReleased(Key key) override;

        void processKeyHold(Key key) override;
    };
}
