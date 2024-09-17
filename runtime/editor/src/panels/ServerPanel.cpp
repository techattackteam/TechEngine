#include "ServerPanel.hpp"


#include "project/ProjectManager.hpp"
#include "LoggerPanel.hpp"

namespace TechEngine {
    ServerPanel::ServerPanel(SystemsRegistry& editorSystemsRegistry,
                             SystemsRegistry& serverSystemsRegistry,
                             LoggerPanel& loggerPanel) : RuntimePanel(editorSystemsRegistry,
                                                                      serverSystemsRegistry,
                                                                      loggerPanel) {
        m_projectType = ProjectType::Server;
    }
}
