#include "SystemsRegistry.hpp"

#include "CopyableSystem.hpp"

namespace TechEngine {
    SystemsRegistry& SystemsRegistry::operator=(const SystemsRegistry& rhs) {
        if (this == &rhs) {
            return *this;
        }
        m_systemsList.clear();
        m_systems.clear();

        for (const std::shared_ptr<System>& system: rhs.m_systemsList) {
            std::shared_ptr<System> newSystem;
            if (std::dynamic_pointer_cast<CopyableSystem>(system)) {
                newSystem = std::dynamic_pointer_cast<CopyableSystem>(system)->clone();
            } else {
                newSystem = system;
            }
            m_systems[typeid(*newSystem)] = newSystem;
            m_systemsList.emplace_back(newSystem);
        }
        return *this;
    }

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
