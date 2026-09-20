#pragma once

#include <TechEngine/core/scene/Access.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
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

    public:
        ScheduleAccess() = default;

        bool reads(ComponentDenseId type) const;

        bool writes(ComponentDenseId type) const;

        bool touches(ComponentDenseId type) const;

        template<typename Function>
        void forEachWrittenType(Function&& function) const {
            forEachType(m_writeMask, std::forward<Function>(function));
        }

        template<typename Function>
        void forEachReadType(Function&& function) const {
            forEachType(m_readMask, std::forward<Function>(function));
        }

    private:
        friend class Schedule;
        friend class TaskGraph;

        ScheduleAccess(const ComponentRegistry& registry, std::span<const ComponentTypeId> written, std::span<const ComponentTypeId> readOnly);

        bool conflicts(const ScheduleAccess& other) const;

        template<typename Function>
        static void forEachType(const std::vector<std::uint64_t>& mask, Function&& function) {
            for (std::size_t wordIndex = 0; wordIndex < mask.size(); wordIndex++) {
                std::uint64_t remaining = mask[wordIndex];
                while (remaining != 0) {
                    const std::size_t bitIndex = static_cast<std::size_t>(std::countr_zero(remaining));
                    const std::size_t denseValue = wordIndex * 64 + bitIndex;
                    function(ComponentDenseId(static_cast<std::uint16_t>(denseValue)));
                    remaining &= remaining - 1;
                }
            }
        }
    };
}
