#pragma once
#include <memory>

#include "core/CoreExportDLL.hpp"

namespace TechEngine {
    class CORE_DLL System {
    protected:
        System() = default;

        friend class SystemsRegistry;

    public:
        virtual ~System() = default;

        virtual void init();

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onStop();

        virtual void shutdown();

        virtual std::unique_ptr<System> copy() {
            return nullptr;
        };
    };
}
