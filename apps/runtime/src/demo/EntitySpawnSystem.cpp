#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <demo/EntityDespawned.hpp>
#include <demo/EntitySpawnSystem.hpp>
#include <demo/EntitySpawned.hpp>
#include <demo/RigidBody.hpp>
#include <demo/Velocity.hpp>

namespace TechEngine {
    void EntitySpawnSystem::init(ScheduleRegistration& registration) {
        registration.access(DeclareAccess<Write<>, Read<Transform>>{});
        registration.setSlot(Slot::Terminal);
        registration.onEvent<Landed>([this](Scene& scene, const std::span<const Landed> landings) {
            onLanded(scene, landings);
        });
        registration.onEvent<Pulse>([this](Scene& scene, const std::span<const Pulse> pulses) {
            onPulse(scene, pulses);
        });
    }

    void EntitySpawnSystem::tick(Scene& scene, const SimulationContext&) {
        TE_PROFILER_FUNCTION();
        if (m_entityCount >= TARGET_ENTITY_COUNT) {
            return;
        }

        const PendingEntity entity = scene.getCommands().spawn();
        scene.addComponent<Velocity>(entity);
        scene.addComponent<RigidBody>(entity);
        scene.publish<EntitySpawned>(static_cast<std::uint32_t>(m_entityCount));
        m_entityCount++;
        TE_LOGGER_INFO("EntitySpawnSystem: Spawned entity {0}, total entities: {1}", m_entityCount - 1, m_entityCount);
    }

    std::string_view EntitySpawnSystem::name() const {
        return "EntitySpawnSystem";
    }

    const std::vector<std::size_t>& EntitySpawnSystem::getLandedBatches() const {
        return m_landedBatches;
    }

    const std::vector<std::size_t>& EntitySpawnSystem::getPulseBatches() const {
        return m_pulseBatches;
    }

    bool EntitySpawnSystem::isPulseSequenceIntact() const {
        return m_pulseSequenceIntact;
    }

    void EntitySpawnSystem::onLanded(Scene& scene, const std::span<const Landed> landings) {
        m_landedBatches.push_back(landings.size());
        for (const Landed& landing: landings) {
            scene.getCommands().despawn(landing.entity);
            scene.publish<EntityDespawned>(landing.entity);
        }
        TE_LOGGER_INFO("EntitySpawnSystem: Destroying {0} landed entities", landings.size());
    }

    void EntitySpawnSystem::onPulse(Scene&, const std::span<const Pulse> pulses) {
        m_pulseBatches.push_back(pulses.size());
        for (const Pulse& pulse: pulses) {
            if (pulse.sequence != m_nextPulse) {
                m_pulseSequenceIntact = false;
            }
            m_nextPulse = pulse.sequence + 1;
        }
    }
}
