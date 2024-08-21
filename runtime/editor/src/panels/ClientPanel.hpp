#pragma once
#include "DockPanel.hpp"
#include "TestPanel.hpp"


namespace TechEngine {
    class ClientPanel : public DockPanel {
    private:
        TestPanel m_TestPanel;

    public:
        void onInit() override;

        void onUpdate() override;
    };
}
