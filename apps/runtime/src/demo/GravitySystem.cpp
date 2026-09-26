#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include "TechEngine/base/diagnostics/Log.hpp"
#include <demo/GravitySystem.hpp>

namespace TechEngine {
    void GravitySystem::init(ScheduleRegistration& registration) {
        registration.access(DeclareAccess<Write<Velocity>, Read<>>{});
        registration.setPriority(20);
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
}
