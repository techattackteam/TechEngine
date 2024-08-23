#pragma once
#include "Panel.hpp"


namespace TechEngine {
    class InspectorPanel : public Panel {
    public:
        void onInit() override;

        void onUpdate() override;
    };
}
