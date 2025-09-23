#include "SystemsRegistry.hpp"

#include "profiling/ProfiledScope.hpp"
#include "profiling/Profiler.hpp"


namespace TechEngine {
    void SystemsRegistry::onStart() {
        for (const std::shared_ptr<System>& system: m_systemsList) {
            system->onStart();
        }
    }

    void SystemsRegistry::onUpdate() {
        Profiler* profiler = nullptr;
        if (hasSystem<Profiler>()) {
            profiler = &getSystem<Profiler>();
        }
        for (const std::shared_ptr<System>& system: m_systemsList) {
            if (profiler) {
                ProfiledScope scope(*profiler, typeid(*system).name());
                system->onUpdate();
            } else {
                system->onUpdate();
            }
        }
    }

    void SystemsRegistry::onFixedUpdate() {
        for (const std::shared_ptr<System>& system: m_systemsList) {
            system->onFixedUpdate();
        }
        return;

        for (const std::shared_ptr<System>& system: m_systemsList) {
            const auto& type_idx = typeid(*system);

            // No need to initialize here, onUpdate will have already done it

            auto startTime = std::chrono::high_resolution_clock::now();
            system->onFixedUpdate();
            auto endTime = std::chrono::high_resolution_clock::now();

            float duration = std::chrono::duration<float, std::milli>(endTime - startTime).count();
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
