#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <demo/CollisionSystem.hpp>

namespace TechEngine {
    void CollisionSystem::init(ScheduleRegistration& registration) {
        registration.access(DeclareAccess<Write<RigidBody>, Read<Transform>>{});
        registration.setPriority(40);
    }

    void CollisionSystem::tick(Scene& scene, const SimulationContext&) {
        TE_PROFILER_SCOPE("CollisionSystem::tick");
        if (!m_query.has_value()) {
            m_query.emplace(scene.query<Write<RigidBody>, Read<Transform>>());
        }
        m_query->each([](const Entity, RigidBody& rigidBody, const Transform& transform) {
            rigidBody.grounded = transform.getWorld().position.y <= 0.0f;
        });
    }

    std::string_view CollisionSystem::name() const {
        return "CollisionSystem";
    }
}
