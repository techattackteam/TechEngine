#include "ClientPanel.hpp"

#include "LoggerPanel.hpp"

namespace TechEngine {
    ClientPanel::ClientPanel(SystemsRegistry& editorSystemsRegistry,
                             SystemsRegistry& clientSystemsRegistry,
                             LoggerPanel& loggerPanel) : RuntimePanel(editorSystemsRegistry,
                                                                      clientSystemsRegistry,
                                                                      loggerPanel) {
        m_projectType = ProjectType::Client;
    }
}
