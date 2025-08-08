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
    };
}
