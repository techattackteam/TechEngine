#pragma once

#include "DockPanel.hpp"
#include "TestPanel.hpp"

namespace TechEngine {
    class ServerPanel : public DockPanel {
    private:
        TestPanel m_TestPanel;

    public:
        void onInit() override;

        void onUpdate() override;

    protected:
        void setupInitialDockingLayout() override;

        void createToolBar() override;
    };
}
