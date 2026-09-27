#pragma once

#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ISystem.hpp>

#include <demo/Landed.hpp>
#include <demo/Pulse.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace TechEngine {
    class EntitySpawnSystem final : public ISystem {
    private:
        static constexpr std::size_t TARGET_ENTITY_COUNT = 100;
        std::size_t m_entityCount = 0;
        std::optional<Query<Write<>, Read<Transform>>> m_query;
        std::vector<std::size_t> m_landedBatches;
        std::vector<std::size_t> m_pulseBatches;
        std::uint32_t m_nextPulse = 0;
        bool m_pulseSequenceIntact = true;

    public:
        void init(ScheduleRegistration& registration) override;

        void tick(Scene& scene, const SimulationContext& context) override;

        std::string_view name() const override;

        const std::vector<std::size_t>& getLandedBatches() const;

        const std::vector<std::size_t>& getPulseBatches() const;

        bool isPulseSequenceIntact() const;

    private:
        void onLanded(Scene& scene, std::span<const Landed> landings);

        void onPulse(Scene& scene, std::span<const Pulse> pulses);
    };
}
