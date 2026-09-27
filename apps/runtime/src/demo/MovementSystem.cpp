#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <demo/MovementSystem.hpp>
#include <demo/Pulse.hpp>

namespace TechEngine {
    void MovementSystem::init(ScheduleRegistration& registration) {
        registration.access(DeclareAccess<Write<Transform>, Read<Velocity>>{});
        registration.setPriority(10);
        registration.onEvent<EntityDespawned>([this](Scene& scene, const std::span<const EntityDespawned> despawns) {
            onDespawned(scene, despawns);
        });
    }

    void MovementSystem::tick(Scene& scene, const SimulationContext& context) {
        TE_PROFILER_SCOPE("MovementSystem::tick");
        if (!m_query.has_value()) {
            m_query.emplace(scene.query<Write<Transform>, Read<Velocity>>());
        }
        m_query->each([&context](const Entity, Transform& transform, const Velocity& velocity) {
            TransformValues local = transform.getLocal();
            local.position += velocity.linear * static_cast<float>(context.fixedDeltaTime);
            (void)transform.setLocal(local);
        });
        for (std::uint32_t i = 0; i < PULSES_PER_TICK; i++) {
            scene.publish<Pulse>(m_nextPulse++);
        }
    }

    std::string_view MovementSystem::name() const {
        return "MovementSystem";
    }

    const std::vector<Entity>& MovementSystem::getDespawned() const {
        return m_despawned;
    }

    void MovementSystem::onDespawned(Scene&, const std::span<const EntityDespawned> despawns) {
        for (const EntityDespawned& despawn: despawns) {
            m_despawned.push_back(despawn.entity);
        }
    }
}
