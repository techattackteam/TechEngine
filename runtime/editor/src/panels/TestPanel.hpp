#pragma once
#include "Panel.hpp"

namespace TechEngine {
    class TestPanel : public Panel {
    public:
        void onInit() override;

        void onUpdate() override;
    };
}
