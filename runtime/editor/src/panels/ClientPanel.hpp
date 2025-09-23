#pragma once
#include "ProfilerPanel.hpp"
#include "RuntimePanel.hpp"
#include "UIEditor.hpp"

namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class ClientPanel : public RuntimePanel {
    private:
        UIEditor m_uiEditor;
        GameView m_gameView;
        ProfilerPanel m_profiler;

    public:
        ClientPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& clientSystemsRegistry, LoggerPanel& loggerPanel);

        ~ClientPanel() override = default;

        void onInit() override;

        void onUpdate() override;

        void setupInitialDockingLayout() override;
    };
}
