#pragma once

#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    using SystemFactory = std::unique_ptr<ISystem> (*)();

    struct OrderConstraint {
        std::type_index systemType;
        Order order = Order::Before;

        bool operator==(const OrderConstraint&) const = default;
    };

    struct ScheduleEntry {
        SystemFactory factory = nullptr;
        std::type_index systemType = typeid(void);
        ScheduleAccess access;
        std::vector<OrderConstraint> orderConstraints;
        int priority = 0;
        Slot slot = Slot::Regular;
    };

    class Schedule {
    private:
        const ComponentRegistry* m_componentRegistry = nullptr;
        std::vector<ScheduleEntry> m_entries;
        std::unordered_map<std::type_index, std::size_t> m_entryByType;
        bool m_frozen = false;

    public:
        explicit Schedule(const ComponentRegistry& componentRegistry);

        Schedule(const Schedule&) = delete;

        Schedule& operator=(const Schedule&) = delete;

        template<std::derived_from<ISystem> T, typename... Written, typename... ReadOnly>
            requires std::default_initializable<T> && ((!std::same_as<Written, Hierarchy>) && ...)
        ScheduleRegistration add(DeclareAccess<Write<Written...>, Read<ReadOnly...>> = {}) {
            const std::array<ComponentTypeId, sizeof...(Written)> written{componentTypeId<Written>()...};
            const std::array<ComponentTypeId, sizeof...(ReadOnly)> readOnly{componentTypeId<ReadOnly>()...};
            return addEntry(typeid(T), &createSystem<T>, ScheduleAccess(*m_componentRegistry, written, readOnly));
        }

        std::span<const ScheduleEntry> getEntries() const;

        std::unordered_map<std::type_index, std::size_t> getEntryByType() const;

        void freeze();

        bool frozen() const;

    private:
        template<std::derived_from<ISystem> T>
            requires std::default_initializable<T>
        static std::unique_ptr<ISystem> createSystem() {
            return std::make_unique<T>();
        }

        ScheduleRegistration addEntry(std::type_index systemType, SystemFactory factory, ScheduleAccess access);

        void setPriority(std::size_t entryIndex, int value);

        void setSlot(std::size_t entryIndex, Slot value);

        void addOrder(std::size_t entryIndex, std::type_index systemType, Order order);

        friend class ScheduleRegistration;
    };
}
