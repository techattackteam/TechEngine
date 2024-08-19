#pragma once

#include "systems/SystemsRegistry.hpp"

namespace TechEngineAPI {
    class Timer;

    class Entry {
    private:
    public:
        static void init(TechEngine::SystemsRegistry* systemsRegistry);

        static void shutdown();
    };
}
