#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <demo/CollisionSystem.hpp>
#include <demo/Landed.hpp>

namespace TechEngine {
    void CollisionSystem::init(ScheduleRegistration& registration) {
        registration.access(DeclareAccess<Write<RigidBody>, Read<Transform>>{});
        registration.setPriority(40);
        registration.onEvent<EntitySpawned>([this](Scene& scene, const std::span<const EntitySpawned> spawns) {
            onSpawned(scene, spawns);
        });
    }

    void CollisionSystem::tick(Scene& scene, const SimulationContext& context) {
        TE_PROFILER_SCOPE("CollisionSystem::tick");
        if (!m_query.has_value()) {
            m_query.emplace(scene.query<Write<RigidBody>, Read<Transform>>());
        }
        m_query->each([&scene](const Entity entity, RigidBody& rigidBody, const Transform& transform) {
            const bool wasGrounded = rigidBody.grounded;
            rigidBody.grounded = transform.getWorld().position.y <= 0.0f;
            if (!wasGrounded && rigidBody.grounded) {
                scene.publish<Landed>(entity);
                TE_LOGGER_INFO("CollisionSystem: Entity {0} (generation {1}) has landed", entity.index, entity.generation);
            }
        });
    }

    std::string_view CollisionSystem::name() const {
        return "CollisionSystem";
    }

    const std::vector<std::uint32_t>& CollisionSystem::getSpawnOrdinals() const {
        return m_spawnOrdinals;
    }

    void CollisionSystem::onSpawned(Scene&, const std::span<const EntitySpawned> spawns) {
        for (const EntitySpawned& spawn: spawns) {
            m_spawnOrdinals.push_back(spawn.ordinal);
        }
    }
}
