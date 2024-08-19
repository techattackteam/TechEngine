#pragma once

#include "systems/SystemsRegistry.hpp"

namespace TechEngineAPI {
    class Entry {
    public:
        virtual ~Entry() = default;

        virtual void init(TechEngine::SystemsRegistry* systemsRegistry);

        virtual void shutdown();
    };
}
