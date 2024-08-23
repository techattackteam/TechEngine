#pragma once
#include "Panel.hpp"


namespace TechEngine {
    class SceneView : public Panel {
    public:
        void onInit() override;
        void onUpdate() override;
    };
}
