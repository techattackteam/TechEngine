#include "ServerPanel.hpp"

namespace TechEngine {
    void ServerPanel::onInit() {
        m_TestPanel.init("Server Test Panel", &m_dockSpaceWindowClass);
    }

    void ServerPanel::onUpdate() {
        m_TestPanel.update();
    }
}
