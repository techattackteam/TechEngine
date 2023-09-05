#pragma once

#include "Panel.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    class GameView : public Panel {
    private:
        Renderer *renderer;
        uint32_t frameBufferID;
    public:
        GameView(Renderer &renderer);

        void onUpdate() override;
    };
}
