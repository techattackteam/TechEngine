#pragma once

#include "Panel.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    class RendererPanel : public Panel {
        Renderer *renderer;
    public:
        RendererPanel(Renderer &renderer);

        void onUpdate() override;

    };
}


