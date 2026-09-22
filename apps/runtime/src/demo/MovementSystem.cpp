#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/scene/Scene.hpp>

#include <demo/MovementSystem.hpp>

namespace TechEngine {
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
    }

    std::string_view MovementSystem::name() const {
        return "MovementSystem";
    }
}
