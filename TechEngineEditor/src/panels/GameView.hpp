#pragma once

#include "Panel.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    class GameView : public Panel {
    private:
        uint32_t frameBufferID;
        SystemsRegistry& appRegistry;

    public:
        GameView(SystemsRegistry& appRegistry);

        void init() override;

        void onUpdate() override;
    };
}
