#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/systems/Schedule.hpp>

#include <utility>

namespace TechEngine {
    Schedule::Schedule(const ComponentRegistry& componentRegistry) : m_componentRegistry(&componentRegistry) {
    }

    std::span<const ScheduleEntry> Schedule::entries() const {
        return m_entries;
    }

    void Schedule::freeze() {
        m_frozen = true;
    }

    bool Schedule::frozen() const {
        return m_frozen;
    }

    ScheduleRegistration Schedule::addEntry(const std::type_index systemType, const SystemFactory factory, ScheduleAccess access) {
        // TODO(S6-T6): reject frozen and duplicate registration before publishing the entry.
        TE_ASSERT(!m_frozen, "Cannot modify schedule after freezing");
        const std::size_t entryIndex = m_entries.size();
        m_entries.push_back({factory, systemType, std::move(access), {}, 0, Slot::Regular});
        m_entryByType.emplace(systemType, entryIndex);
        return ScheduleRegistration(*this, entryIndex);
    }

    void Schedule::setPriority(const std::size_t entryIndex, const int value) {
        TE_ASSERT(!m_frozen, "Cannot modify schedule after freezing");
        TE_ASSERT(entryIndex < m_entries.size(), "Invalid schedule entry index");
        m_entries.at(entryIndex).priority = value;
    }

    void Schedule::setSlot(const std::size_t entryIndex, const Slot value) {
        TE_ASSERT(!m_frozen, "Cannot modify schedule after freezing");
        TE_ASSERT(entryIndex < m_entries.size(), "Invalid schedule entry index");
        m_entries.at(entryIndex).slot = value;
    }

    void Schedule::addOrder(const std::size_t entryIndex, const std::type_index systemType, const Order order) {
        TE_ASSERT(!m_frozen, "Cannot modify schedule after freezing");
        m_entries.at(entryIndex).orderConstraints.push_back({systemType, order});
    }
}
