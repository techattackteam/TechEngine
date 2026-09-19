#pragma once

#include <TechEngine/core/systems/Schedule.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <typeindex>
#include <vector>

namespace TechEngine {
    struct TaskGraphNode {
        SystemFactory factory = nullptr;
        std::type_index systemType = typeid(void);
        ScheduleAccess access;
    };

    using TaskGraphLevel = std::vector<TaskGraphNode>;

    class TaskGraph {
    private:
        std::vector<TaskGraphLevel> m_levels;

        void addEdge(std::vector<std::vector<std::size_t>>& edges, std::vector<std::size_t>& indegree, std::size_t before, std::size_t after);

        bool findCycleFrom(std::size_t node, const std::vector<std::vector<std::size_t>>& edges, std::vector<std::uint8_t>& state, std::vector<std::size_t>& path, std::vector<std::size_t>& cycle);

        std::vector<std::size_t> findCycle(const std::vector<std::vector<std::size_t>>& edges);

        std::string cycleMessage(const std::vector<std::size_t>& cycle, const std::vector<std::string>& systemNames);

    public:
        explicit TaskGraph(Schedule& schedule);

        std::span<const TaskGraphLevel> getLevels() const;
    };
}
