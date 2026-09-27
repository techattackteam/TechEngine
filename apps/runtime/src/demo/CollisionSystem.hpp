#pragma once

#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ISystem.hpp>

#include <demo/EntitySpawned.hpp>
#include <demo/RigidBody.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace TechEngine {
    class CollisionSystem final : public ISystem {
    private:
        std::optional<Query<Write<RigidBody>, Read<Transform>>> m_query;
        std::vector<std::uint32_t> m_spawnOrdinals;

    public:
        void init(ScheduleRegistration& registration) override;

        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;

        const std::vector<std::uint32_t>& getSpawnOrdinals() const;

    private:
        void onSpawned(Scene& scene, std::span<const EntitySpawned> spawns);
    };
}
