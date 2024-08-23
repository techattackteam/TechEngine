#pragma once

#include "DockPanel.hpp"
#include "TestPanel.hpp"


namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class ClientPanel : public DockPanel {
    private:
        TestPanel m_TestPanel;
        SystemsRegistry& m_systemRegistry;
        SystemsRegistry& m_clientSystemsRegistry;
        LoggerPanel& loggerPanel;

    public:
        ClientPanel(SystemsRegistry& systemsRegistry, SystemsRegistry& clientSystemsRegistry, LoggerPanel& loggerPanel);

        void onInit() override;

        void onUpdate() override;

    protected:
        void setupInitialDockingLayout() override;

        void startRunningScene();

        void stopRunningScene();

        void createToolBar() override;
    };
}
