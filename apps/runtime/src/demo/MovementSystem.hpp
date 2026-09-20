#pragma once

#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ISystem.hpp>

#include <demo/Velocity.hpp>

#include <optional>

namespace TechEngine {
    class MovementSystem final : public ISystem {
    private:
        std::optional<Query<Write<Transform>, Read<Velocity>>> m_query;

    public:
        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;
    };
}
