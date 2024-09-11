#pragma once

#include "DockPanel.hpp"

namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class ServerPanel : public DockPanel {
    private:
        SystemsRegistry& m_systemRegistry;
        SystemsRegistry& m_serverSystemsRegistry;
        LoggerPanel& loggerPanel;

    public:
        ServerPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& serverSystemsRegistry, LoggerPanel& loggerPanel);

        void onInit() override;

        void onUpdate() override;

    protected:
        void setupInitialDockingLayout() override;

        void startRunningScene();

        void stopRunningScene();

        void createToolBar() override;
    };
}
