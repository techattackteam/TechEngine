#pragma once
#include "Panel.hpp"


namespace TechEngine {
    class GameView : public Panel {
    public:
        void onInit() override;

        void onUpdate() override;
    };
}
