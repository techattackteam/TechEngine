#pragma once

#include "RuntimePanel.hpp"

namespace TechEngine {
    class ServerPanel : public RuntimePanel {
    public:
        ServerPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& serverSystemsRegistry, LoggerPanel& loggerPanel);

        ~ServerPanel() override = default;
    };
}
