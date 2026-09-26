#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneCommandBuffer.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>
#include <TechEngine/core/systems/SerialExecutor.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace TechEngine {
    class SerialExecutor::Impl {
    public:
        struct Node {
            ScheduleAccess access;
            ISystem* system = nullptr;
            SceneCommandBuffer commands;
        };

        using Level = std::vector<Node>;

        std::vector<Level> levels;
        std::vector<Entity> spawned;
    };

    SerialExecutor::SerialExecutor(const TaskGraph& graph) : m_impl(std::make_unique<Impl>()) {
        for (const TaskGraphLevel& sourceLevel: graph.getLevels()) {
            Impl::Level level;
            level.reserve(sourceLevel.size());
            for (const TaskGraphNode& sourceNode: sourceLevel) {
                level.push_back({sourceNode.access, sourceNode.system, {}});
            }
            m_impl->levels.push_back(std::move(level));
        }
    }

    SerialExecutor::~SerialExecutor() = default;

    void SerialExecutor::execute(Scene& scene, const SimulationContext& context, TickBarrierServices& barrier) {
        TE_PROFILER_FUNCTION();
        try {
            {
                TE_PROFILER_SCOPE("SerialExecutor.Systems");
                for (Impl::Level& level: m_impl->levels) {
                    for (Impl::Node& node: level) {
                        scene.beginSystem(node.access, node.commands, context.tick);
                        try {
                            node.system->tick(scene, context);
                        } catch (...) {
                            scene.endSystem();
                            throw;
                        }
                        scene.endSystem();
                    }
                }
            }
        } catch (...) {
            for (Impl::Level& level: m_impl->levels) {
                for (Impl::Node& node: level) {
                    node.commands.discard();
                }
            }
            throw;
        }

        {
            TE_PROFILER_SCOPE("SerialExecutor.ApplyCommands");
            m_impl->spawned.clear();
            for (Impl::Level& level: m_impl->levels) {
                for (Impl::Node& node: level) {
                    scene.applyCommands(node.commands, m_impl->spawned);
                }
            }
        }
        {
            TE_PROFILER_SCOPE("SerialExecutor.BarrierServices");
            barrier.assignNetIds(scene, m_impl->spawned);
            barrier.flushEvents(context.tick);
        }
    }

}
