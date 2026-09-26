#pragma once

#include <string_view>

namespace TechEngine {
    class Scene;
    class ScheduleRegistration;
    struct SimulationContext;

    class ISystem {
    public:
        virtual ~ISystem() = default;

        virtual void init(ScheduleRegistration& registration) = 0;

        virtual void tick(Scene& scene, const SimulationContext& context) = 0;

        virtual std::string_view name() const = 0;
    };
}
