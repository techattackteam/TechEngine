#pragma once

#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ISystem.hpp>

#include <cstddef>
#include <optional>

namespace TechEngine {
    class EntitySpawnSystem final : public ISystem {
    private:
        static constexpr std::size_t TARGET_ENTITY_COUNT = 100;
        std::size_t m_entityCount = 0;
        std::optional<Query<Write<>, Read<Transform>>> m_query;

    public:
        void init(ScheduleRegistration& registration) override;

        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;
    };
}
