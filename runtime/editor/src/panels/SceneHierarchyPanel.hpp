#pragma once
#include "Panel.hpp"


namespace TechEngine {
    class SceneHierarchyPanel : public Panel {
    public:
        void onInit() override;

        void onUpdate() override;
    };
}
