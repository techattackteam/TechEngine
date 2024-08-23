#pragma once

#include "DockPanel.hpp"
#include "TestPanel.hpp"

namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class ServerPanel : public DockPanel {
    private:
        TestPanel m_TestPanel;
        SystemsRegistry& m_systemRegistry;
        SystemsRegistry& m_serverSystemsRegistry;
        LoggerPanel& loggerPanel;

    public:
        ServerPanel(SystemsRegistry& systemsRegistry, SystemsRegistry& serverSystemsRegistry, LoggerPanel& loggerPanel);

        void onInit() override;

        void onUpdate() override;

    protected:
        void setupInitialDockingLayout() override;

        void startRunningScene();

        void stopRunningScene();

        void createToolBar() override;
    };
}
