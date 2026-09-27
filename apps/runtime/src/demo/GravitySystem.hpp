#pragma once

#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/systems/ISystem.hpp>

#include <demo/EntityDespawned.hpp>
#include <demo/Landed.hpp>
#include <demo/Velocity.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace TechEngine {
    class GravitySystem final : public ISystem {
    private:
        std::optional<Query<Write<Velocity>, Read<>>> m_query;
        std::vector<std::size_t> m_landedBatches;
        std::vector<Entity> m_despawned;

    public:
        void init(ScheduleRegistration& registration) override;

        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;

        const std::vector<std::size_t>& getLandedBatches() const;

        const std::vector<Entity>& getDespawned() const;

    private:
        void onLanded(Scene& scene, std::span<const Landed> landings);

        void onDespawned(Scene& scene, std::span<const EntityDespawned> despawns);
    };
}
