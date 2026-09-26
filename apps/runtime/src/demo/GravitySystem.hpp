#pragma once

#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/systems/ISystem.hpp>

#include <demo/Velocity.hpp>

#include <optional>

namespace TechEngine {
    class GravitySystem final : public ISystem {
    private:
        std::optional<Query<Write<Velocity>, Read<>>> m_query;

    public:
        void init(ScheduleRegistration& registration) override;

        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;
    };
}
