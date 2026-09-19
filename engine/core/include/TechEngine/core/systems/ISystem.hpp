#pragma once

#include <string_view>

namespace TechEngine {
    class Scene;
    struct SimulationContext;

    class ISystem {
    public:
        virtual ~ISystem() = default;

        virtual void update(Scene& scene, const SimulationContext& context) = 0;

        virtual std::string_view name() const = 0;
    };
}
