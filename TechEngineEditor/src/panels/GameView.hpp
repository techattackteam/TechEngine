#pragma once

#include "Panel.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    class GameView : public Panel {
    private:
        Renderer* renderer;
        Scene& scene;
        uint32_t frameBufferID;

    public:
        GameView(EventDispatcher& eventDispatcher, Renderer& renderer, Scene& scene);

        void onUpdate() override;
    };
}
