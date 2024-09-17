#pragma once
#include "RuntimePanel.hpp"


namespace TechEngine {
    class LoggerPanel;
    class SystemsRegistry;

    class ClientPanel : public RuntimePanel {
    public:
        ClientPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& clientSystemsRegistry, LoggerPanel& loggerPanel);

        ~ClientPanel() override = default;
    };
}
