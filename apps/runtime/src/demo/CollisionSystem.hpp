#pragma once

#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ISystem.hpp>

#include <demo/RigidBody.hpp>

#include <optional>

namespace TechEngine {
    class CollisionSystem final : public ISystem {
    private:
        std::optional<Query<Write<RigidBody>, Read<Transform>>> m_query;

    public:
        void init(ScheduleRegistration& registration) override;

        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;
    };
}
