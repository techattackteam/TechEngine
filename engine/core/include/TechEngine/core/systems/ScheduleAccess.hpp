#pragma once

#include <TechEngine/core/scene/Access.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace TechEngine {
    class ComponentRegistry;
    class Schedule;

    template<typename WritableTypes, typename ReadableTypes>
    struct DeclareAccess;

    template<typename... Written, typename... ReadOnly>
    struct DeclareAccess<Write<Written...>, Read<ReadOnly...>> {};

    class ScheduleAccess {
    private:
        std::vector<std::uint64_t> m_readMask;
        std::vector<std::uint64_t> m_writeMask;

        ScheduleAccess(const ComponentRegistry& registry, std::span<const ComponentTypeId> written, std::span<const ComponentTypeId> readOnly);

        friend class Schedule;

    public:
        ScheduleAccess() = default;

        bool reads(ComponentDenseId type) const;

        bool writes(ComponentDenseId type) const;

        bool touches(ComponentDenseId type) const;
    };
}
