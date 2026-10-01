#pragma once

#include <TechEngine/core/events/EventTypeId.hpp>
#include <TechEngine/core/systems/EventHandler.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <typeindex>
#include <vector>

namespace TechEngine {
    class EventRegistry;

    struct TaskGraphEventHandler {
        EventTypeId eventType;
        EventCallback handler;
    };

    struct TaskGraphNode {
        ISystem* system = nullptr;
        std::type_index systemType = typeid(void);
        ScheduleAccess access;
        std::vector<TaskGraphEventHandler> eventHandlers;
        std::vector<InputHandler> inputHandlers;
    };

    using TaskGraphLevel = std::vector<TaskGraphNode>;

    class TaskGraph {
    private:
        std::vector<TaskGraphLevel> m_levels;

        void addEdge(std::vector<std::vector<std::size_t>>& edges, std::vector<std::size_t>& indegree, std::size_t before, std::size_t after);

        bool findCycleFrom(std::size_t node, const std::vector<std::vector<std::size_t>>& edges, std::vector<std::uint8_t>& state, std::vector<std::size_t>& path, std::vector<std::size_t>& cycle);

        std::vector<std::size_t> findCycle(const std::vector<std::vector<std::size_t>>& edges);

        std::string cycleMessage(const std::vector<std::size_t>& cycle, const std::vector<std::string>& systemNames);

        std::vector<std::vector<TaskGraphEventHandler>> resolveEventHandlers(std::span<const ScheduleEntry> entries, const EventRegistry& events, const std::vector<std::string>& systemNames);

    public:
        TaskGraph(Schedule& schedule, const EventRegistry& events);

        std::span<const TaskGraphLevel> getLevels() const;
    };
}
