#pragma once

#include "Panel.hpp"


namespace TechEngine {
    class RendererPanel : public Panel {
        Window window;

    public:
        RendererPanel();

        void onUpdate() override;

        Window &getWindow();
    };
}


