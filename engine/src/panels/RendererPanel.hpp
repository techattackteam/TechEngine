#pragma once

#include "Panel.hpp"


namespace Engine {
    class RendererPanel : public Panel {
        Window window;

    public:
        RendererPanel();

        void onUpdate() override;

        Window &getWindow();
    };
}


