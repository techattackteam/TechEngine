#pragma once
#include "core/CoreExportDLL.hpp"

namespace TechEngine {
    class CORE_DLL System {
    protected:
        System() = default;

        friend class SystemsRegistry;

    public:
        System(const System&) = delete;

        System& operator=(const System&) = delete;

        virtual ~System() = default;

        virtual void init() = 0;

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onStop();

        virtual void shutdown() = 0;
    };
}
