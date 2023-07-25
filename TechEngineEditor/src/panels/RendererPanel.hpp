#pragma once

#include "Panel.hpp"
#include "renderer/Renderer.hpp"
#include "UIUtils/Guizmo.hpp"

namespace TechEngine {
    class RendererPanel : public Panel {
        Renderer *renderer;
        Guizmo guizmo;
    public:
        RendererPanel(Renderer &renderer);

        void onUpdate() override;

    };
}


