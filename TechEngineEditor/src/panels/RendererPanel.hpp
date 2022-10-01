#pragma once

#include "Panel.hpp"


namespace TechEngine {
    class RendererPanel : public Panel {
        //Renderer renderer;

    public:
        RendererPanel();

        void onUpdate() override;

    };
}


