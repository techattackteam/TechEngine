#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/Scene.hpp>

#include "TechEngine/base/diagnostics/Log.hpp"
#include <demo/EntitySpawnSystem.hpp>
#include <demo/RigidBody.hpp>
#include <demo/Velocity.hpp>

namespace TechEngine {
    void EntitySpawnSystem::tick(Scene& scene, const SimulationContext&) {
        TE_PROFILER_FUNCTION();
        if (m_entityCount >= TARGET_ENTITY_COUNT) {
            return;
        }

        const PendingEntity entity = scene.getCommands().spawn();
        scene.addComponent<Velocity>(entity);
        scene.addComponent<RigidBody>(entity);
        m_entityCount++;
        TE_LOGGER_INFO("EntitySpawnSystem: Spawned entity {0}, total entities: {1}", m_entityCount - 1, m_entityCount);
    }

    std::string_view EntitySpawnSystem::name() const {
        return "EntitySpawnSystem";
    }
}
