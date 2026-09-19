#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

namespace TechEngine {
    ScheduleRegistration::ScheduleRegistration(Schedule& schedule, const std::size_t entryIndex) : m_schedule(&schedule), m_entryIndex(entryIndex) {
    }

    ScheduleRegistration& ScheduleRegistration::priority(const int value) {
        m_schedule->setPriority(m_entryIndex, value);
        return *this;
    }

    ScheduleRegistration& ScheduleRegistration::slot(const Slot value) {
        m_schedule->setSlot(m_entryIndex, value);
        return *this;
    }

    void ScheduleRegistration::addOrder(const std::type_index systemType, const Order order) {
        m_schedule->addOrder(m_entryIndex, systemType, order);
    }
}
