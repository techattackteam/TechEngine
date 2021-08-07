#pragma once

#include "Panel.hpp"

namespace Engine {
    class RendererPanel : public Panel {
    public:
        RendererPanel();

        void onUpdate() override;
    };
}


