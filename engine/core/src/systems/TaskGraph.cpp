#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TechEngine {
    TaskGraph::TaskGraph(Schedule& schedule) {
        const std::span<const ScheduleEntry> entries = schedule.getEntries();
        const std::unordered_map<std::type_index, std::size_t> entryByType = schedule.getEntryByType();

        std::vector<std::string> systemNames;
        systemNames.reserve(entries.size());
        for (const ScheduleEntry& entry: entries) {
            const std::unique_ptr<ISystem> system = entry.factory();
            systemNames.emplace_back(system->name());
        }

        for (std::size_t i = 0; i < entries.size(); i++) {
            const ScheduleEntry& entry = entries[i];
            for (const OrderConstraint& constraint: entry.orderConstraints) {
                TE_CHECK(entryByType.contains(constraint.systemType), "System {0} has an ordering constraint on unregistered system {1}", systemNames[i], constraint.systemType.name());
            }
        }

        std::vector<std::vector<std::size_t>> edges(entries.size());
        std::vector<std::size_t> indegree(entries.size(), 0);
        std::vector<std::vector<bool>> explicitlyOrdered(entries.size(), std::vector<bool>(entries.size(), false));

        for (std::size_t i = 0; i < entries.size(); i++) {
            for (const OrderConstraint& constraint: entries[i].orderConstraints) {
                const std::size_t target = entryByType.at(constraint.systemType);
                explicitlyOrdered[i][target] = true;
                explicitlyOrdered[target][i] = true;
                if (constraint.order == Order::Before) {
                    addEdge(edges, indegree, i, target);
                } else {
                    addEdge(edges, indegree, target, i);
                }
            }
        }

        for (std::size_t terminal = 0; terminal < entries.size(); terminal++) {
            if (entries[terminal].slot != Slot::Terminal) {
                continue;
            }
            for (std::size_t regular = 0; regular < entries.size(); regular++) {
                if (entries[regular].slot == Slot::Regular) {
                    explicitlyOrdered[regular][terminal] = true;
                    explicitlyOrdered[terminal][regular] = true;
                    addEdge(edges, indegree, regular, terminal);
                }
            }
        }

        for (std::size_t first = 0; first < entries.size(); first++) {
            for (std::size_t second = first + 1; second < entries.size(); second++) {
                if (!entries[first].access.conflicts(entries[second].access) || explicitlyOrdered[first][second]) {
                    continue;
                }

                TE_CHECK(entries[first].priority != entries[second].priority, "Conflicting systems {0} and {1} have equal priority {2}", systemNames[first], systemNames[second], entries[first].priority);
                const bool firstRunsFirst = entries[first].priority < entries[second].priority;
                const std::size_t before = firstRunsFirst ? first : second;
                const std::size_t after = firstRunsFirst ? second : first;
                addEdge(edges, indegree, before, after);
                TE_LOGGER_INFO("Task graph conflict: {0} -> {1}", systemNames[before], systemNames[after]);
            }
        }

        std::vector<bool> processed(entries.size(), false);
        std::size_t processedCount = 0;
        while (processedCount < entries.size()) {
            std::vector<std::size_t> ready;
            for (std::size_t i = 0; i < entries.size(); i++) {
                if (!processed[i] && indegree[i] == 0) {
                    ready.push_back(i);
                }
            }

            if (ready.empty()) {
                const std::vector<std::size_t> cycle = findCycle(edges);
                const std::string message = cycleMessage(cycle, systemNames);
                TE_CHECK(false, "{0}", message);
                return;
            }

            TaskGraphLevel level;
            level.reserve(ready.size());
            for (const std::size_t node: ready) {
                processed[node] = true;
                processedCount++;
                level.push_back({entries[node].factory, entries[node].systemType, entries[node].access});
            }
            for (const std::size_t node: ready) {
                for (const std::size_t target: edges[node]) {
                    indegree[target]--;
                }
            }
            m_levels.push_back(std::move(level));
        }

        schedule.freeze();
    }

    void TaskGraph::addEdge(std::vector<std::vector<std::size_t>>& edges, std::vector<std::size_t>& indegree, const std::size_t before, const std::size_t after) {
        if (std::find(edges[before].begin(), edges[before].end(), after) != edges[before].end()) {
            return;
        }
        edges[before].push_back(after);
        indegree[after]++;
    }

    bool TaskGraph::findCycleFrom(const std::size_t node, const std::vector<std::vector<std::size_t>>& edges, std::vector<std::uint8_t>& state, std::vector<std::size_t>& path, std::vector<std::size_t>& cycle) {
        state[node] = 1;
        path.push_back(node);

        for (const std::size_t target: edges[node]) {
            if (state[target] == 0 && findCycleFrom(target, edges, state, path, cycle)) {
                return true;
            }
            if (state[target] == 1) {
                const auto cycleStart = std::find(path.begin(), path.end(), target);
                cycle.assign(cycleStart, path.end());
                return true;
            }
        }

        path.pop_back();
        state[node] = 2;
        return false;
    }

    std::vector<std::size_t> TaskGraph::findCycle(const std::vector<std::vector<std::size_t>>& edges) {
        std::vector<std::uint8_t> state(edges.size(), 0);
        std::vector<std::size_t> path;
        std::vector<std::size_t> cycle;
        for (std::size_t i = 0; i < edges.size(); i++) {
            if (state[i] == 0 && findCycleFrom(i, edges, state, path, cycle)) {
                return cycle;
            }
        }
        return cycle;
    }

    std::string TaskGraph::cycleMessage(const std::vector<std::size_t>& cycle, const std::vector<std::string>& systemNames) {
        std::string message = "Task graph cycle: ";
        for (std::size_t i = 0; i < cycle.size(); i++) {
            if (i != 0) {
                message += " -> ";
            }
            message += systemNames[cycle[i]];
        }
        if (!cycle.empty()) {
            message += " -> ";
            message += systemNames[cycle.front()];
        }
        return message;
    }

    std::span<const TaskGraphLevel> TaskGraph::levels() const {
        return m_levels;
    }
}
