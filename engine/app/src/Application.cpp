#include "Application.hpp"

namespace TechEngine {
    void Application::run() {
        while (m_running) {
            m_runFunction();
        }
    }
}
