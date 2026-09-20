#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstdint>
#include <memory>
#include <span>

namespace TechEngine {
    class Scene;
    class TaskGraph;
    struct SimulationContext;

    class TickBarrierServices {
    public:
        virtual ~TickBarrierServices() = default;

        virtual void assignNetIds(Scene& scene, std::span<const Entity> spawned) = 0;

        virtual void flushEvents(std::uint64_t frameIndex, std::uint64_t tick) = 0;
    };

    class SerialExecutor {
    private:
        class Impl;

        std::unique_ptr<Impl> m_impl;

    public:
        explicit SerialExecutor(const TaskGraph& graph);

        ~SerialExecutor();

        SerialExecutor(const SerialExecutor&) = delete;

        SerialExecutor(SerialExecutor&&) = delete;

        SerialExecutor& operator=(const SerialExecutor&) = delete;

        SerialExecutor& operator=(SerialExecutor&&) = delete;

        void execute(Scene& scene, const SimulationContext& context /*, TickBarrierServices& barrier*/);
    };
}
