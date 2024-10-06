#include "SystemsRegistry.hpp"


namespace TechEngine {
    void SystemsRegistry::onStart() {
        for (const std::shared_ptr<System>& system: m_systemsList) {
            system->onStart();
        }
    }

    void SystemsRegistry::onUpdate() {
        for (const std::shared_ptr<System>& system: m_systemsList) {
            system->onUpdate();
        }
    }

    void SystemsRegistry::onFixedUpdate() {
        for (const std::shared_ptr<System>& system: m_systemsList) {
            system->onFixedUpdate();
        }
    }

    void SystemsRegistry::onStop() {
        for (const std::shared_ptr<System>& system: m_systemsList) {
            system->onStop();
        }
    }

    void SystemsRegistry::onShutdown() {
        for (const std::shared_ptr<System>& system: m_systemsList) {
            system->shutdown();
        }
    }
}
