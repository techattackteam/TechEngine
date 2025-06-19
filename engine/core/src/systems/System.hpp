#pragma once

#include "core/CoreExportDLL.hpp"

namespace TechEngine {
    class CORE_DLL System {
    protected:
        friend class SystemsRegistry;

        System() = default;

    public:
        virtual ~System() = default;

        // Called when the engine is initializing, before the application starts
        virtual void init();

        // Called when the simulation starts or runtime starts
        virtual void onStart();

        // Called on every fixed update
        virtual void onFixedUpdate();

        // Called on every frame
        virtual void onUpdate();

        // Called when the simulation stops or runtime stops
        virtual void onStop();

        // Called when the engine is shutting down, after the application stops
        virtual void shutdown();
    };
}
