#pragma once
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    class Core {
    protected:
        SystemsRegistry m_systemManager;

    public:
        virtual ~Core() = default;

        virtual void init();

        virtual void onStart() = 0;

        virtual void onFixedUpdate() = 0;

        virtual void onUpdate() = 0;

        virtual void onStop() = 0;

        virtual void destroy() = 0;
    };
}
