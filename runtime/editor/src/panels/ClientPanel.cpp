#include "ClientPanel.hpp"

namespace TechEngine {
    void ClientPanel::onInit() {
        m_TestPanel.init("Client Test Panel", &m_dockSpaceWindowClass);
    }

    void ClientPanel::onUpdate() {
        m_TestPanel.update();
    }
}
