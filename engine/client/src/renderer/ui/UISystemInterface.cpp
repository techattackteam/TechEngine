#include "UISystemInterface.hpp"


namespace TechEngine {
    UISystemInterface::UISystemInterface(Timer* timer) : m_timer(timer) {
        // Constructor implementation
        if (!m_timer) {
            throw std::runtime_error("Timer cannot be null");
        }
    }

    double UISystemInterface::GetElapsedTime() {
        return m_timer->getTime();
    }
}
