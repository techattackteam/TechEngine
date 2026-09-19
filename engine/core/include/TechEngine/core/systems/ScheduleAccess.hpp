#pragma once

#include <TechEngine/core/scene/Access.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace TechEngine {
    class ComponentRegistry;
    class Schedule;
    class TaskGraph;

    template<typename WritableTypes, typename ReadableTypes>
    struct DeclareAccess;

    template<typename... Written, typename... ReadOnly>
    struct DeclareAccess<Write<Written...>, Read<ReadOnly...>> {};

    class ScheduleAccess {
    private:
        std::vector<std::uint64_t> m_readMask;
        std::vector<std::uint64_t> m_writeMask;
        std::vector<ComponentDenseId> m_writtenTypes;

        ScheduleAccess(const ComponentRegistry& registry, std::span<const ComponentTypeId> written, std::span<const ComponentTypeId> readOnly);

        bool conflicts(const ScheduleAccess& other) const;

        friend class Schedule;
        friend class TaskGraph;

    public:
        ScheduleAccess() = default;

        bool reads(ComponentDenseId type) const;

        bool writes(ComponentDenseId type) const;

        bool touches(ComponentDenseId type) const;

        std::span<const ComponentDenseId> getWrittenTypes() const;
    };
}
