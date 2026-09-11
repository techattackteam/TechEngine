#pragma once

#include <TechEngine/app/App.hpp>

namespace TechEngine {
    class RuntimeApp : public App {
    public:
        RuntimeApp();

        ~RuntimeApp() override = default;

        static Role runtimeRole();

    protected:
        void init() override;

        void fixedUpdate(const SimulationContext& simulation) override;
    };
}
