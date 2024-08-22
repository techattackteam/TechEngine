#pragma once
#include "Panel.hpp"


namespace TechEngine {
    class DockPanel : public Panel {
    protected:
        ImGuiID m_dockSpaceID = 0;
        bool m_firstTime = true;

    public:
        ImGuiWindowClass m_dockSpaceWindowClass;

        void onInit() override;

        void update();

    protected:
        virtual void setupInitialDockingLayout() = 0;

        virtual void createToolBar();
    };
}
