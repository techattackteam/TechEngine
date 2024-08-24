#pragma once

#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    class Core {
    public:
        SystemsRegistry m_systemRegistry;

    public:
        virtual ~Core() = default;

        virtual void init();

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onStop();

        virtual void shutdown();
    };
}
