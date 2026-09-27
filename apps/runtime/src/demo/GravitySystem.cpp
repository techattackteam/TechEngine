#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <demo/GravitySystem.hpp>

namespace TechEngine {
    void GravitySystem::init(ScheduleRegistration& registration) {
        registration.access(DeclareAccess<Write<Velocity>, Read<>>{});
        registration.setPriority(20);
        registration.onEvent<Landed>([this](Scene& scene, const std::span<const Landed> landings) {
            onLanded(scene, landings);
        });
        registration.onEvent<EntityDespawned>([this](Scene& scene, const std::span<const EntityDespawned> despawns) {
            onDespawned(scene, despawns);
        });
    }

    void GravitySystem::tick(Scene& scene, const SimulationContext& context) {
        TE_PROFILER_SCOPE("GravitySystem::tick");
        if (!m_query.has_value()) {
            m_query.emplace(scene.query<Write<Velocity>, Read<>>());
        }
        m_query->each([&context](const Entity entity, Velocity& velocity) {
            velocity.linear.y -= 9.81f * static_cast<float>(context.fixedDeltaTime);
        });
    }

    std::string_view GravitySystem::name() const {
        return "GravitySystem";
    }

    const std::vector<std::size_t>& GravitySystem::getLandedBatches() const {
        return m_landedBatches;
    }

    const std::vector<Entity>& GravitySystem::getDespawned() const {
        return m_despawned;
    }

    void GravitySystem::onLanded(Scene&, const std::span<const Landed> landings) {
        m_landedBatches.push_back(landings.size());
    }

    void GravitySystem::onDespawned(Scene&, const std::span<const EntityDespawned> despawns) {
        for (const EntityDespawned& despawn: despawns) {
            m_despawned.push_back(despawn.entity);
        }
    }
}
