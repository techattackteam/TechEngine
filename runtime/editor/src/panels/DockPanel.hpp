#pragma once
#include "Panel.hpp"


namespace TechEngine {
    class DockPanel : public Panel {
    public:
        ImGuiWindowClass m_dockSpaceWindowClass;

        virtual void onInit() override;

        void update();
    };
}
