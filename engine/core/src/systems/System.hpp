#pragma once

#include <iostream>

#include "core/CoreExportDLL.hpp"

namespace TechEngine {
    class CORE_DLL System {
    protected:
        friend class SystemsRegistry;

        System() = default;

    public:
        virtual ~System() = default;

        virtual void init();

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onStop();

        virtual void shutdown();
    };
}
