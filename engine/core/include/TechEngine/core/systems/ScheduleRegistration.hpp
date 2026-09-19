#pragma once

#include <TechEngine/core/systems/ISystem.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <typeindex>

namespace TechEngine {
    enum class Slot : std::uint8_t {
        Regular,
        Terminal,
    };

    enum class Order : std::uint8_t {
        Before,
        After,
    };

    class Schedule;

    class ScheduleRegistration {
    private:
        Schedule* m_schedule = nullptr;
        std::size_t m_entryIndex = 0;

        ScheduleRegistration(Schedule& schedule, std::size_t entryIndex);

        friend class Schedule;

    public:
        ScheduleRegistration& priority(int value);

        ScheduleRegistration& slot(Slot value);

        template<std::derived_from<ISystem> T>
        ScheduleRegistration& before() {
            addOrder(typeid(T), Order::Before);
            return *this;
        }

        template<std::derived_from<ISystem> T>
        ScheduleRegistration& after() {
            addOrder(typeid(T), Order::After);
            return *this;
        }

    private:
        void addOrder(std::type_index systemType, Order order);
    };
}
