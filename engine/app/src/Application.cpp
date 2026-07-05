#include "Application.hpp"

namespace TechEngine {
    void Application::run() const {
        while (m_running) {
            m_runFunction();
        }
    }
}
