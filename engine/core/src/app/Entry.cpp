#include "Entry.hpp"

#include "systems/SystemsRegistry.hpp"
#include "core/Timer.hpp"

#include <functional>

namespace TechEngine {
    Entry::Entry(SystemsRegistry& systemsRegistry)
        : m_systemsRegistry(systemsRegistry) {
    }

    Entry::Entry(const Entry& rhs) : m_systemsRegistry(rhs.m_systemsRegistry) {
    }

    void Entry::run(const std::function<void()>& onFixedUpdate, const std::function<void()>& onUpdate) {
        Timer& timer = m_systemsRegistry.getSystem<Timer>();
        timer.tick();
        timer.addAccumulator(timer.getDeltaTime());

        while (timer.getAccumulator() >= timer.getTPS()) {
            timer.updateTicks();
            onFixedUpdate();
            timer.addAccumulator(-timer.getTPS());
        }

        onUpdate();
        timer.updateInterpolation();
    }
}
