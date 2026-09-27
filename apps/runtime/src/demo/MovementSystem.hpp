#pragma once

#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ISystem.hpp>

#include <demo/EntityDespawned.hpp>
#include <demo/Velocity.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace TechEngine {
    class MovementSystem final : public ISystem {
    private:
        std::optional<Query<Write<Transform>, Read<Velocity>>> m_query;
        std::uint32_t m_nextPulse = 0;
        std::vector<Entity> m_despawned;

    public:
        static constexpr std::uint32_t PULSES_PER_TICK = 500;

        void init(ScheduleRegistration& registration) override;

        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;

        const std::vector<Entity>& getDespawned() const;

    private:
        void onDespawned(Scene& scene, std::span<const EntityDespawned> despawns);
    };
}
