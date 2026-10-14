#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/systems/Schedule.hpp>

#include <utility>

namespace TechEngine {
    Schedule::Schedule(const ComponentRegistry& componentRegistry) : m_componentRegistry(&componentRegistry) {
    }

    std::span<const ScheduleEntry> Schedule::getEntries() const {
        return m_entries;
    }
    std::unordered_map<std::type_index, std::size_t> Schedule::getEntryByType() const {
        return m_entryByType;
    }

    void Schedule::freeze() {
        m_frozen = true;
    }

    bool Schedule::frozen() const {
        return m_frozen;
    }

    ScheduleRegistration Schedule::addEntry(const std::type_index systemType, const SystemFactory factory, ScheduleAccess access) {
        TE_CHECK(!m_frozen, "Cannot modify schedule after freezing");
        TE_CHECK(m_entryByType.find(systemType) == m_entryByType.end(), "Duplicate system type found");
        std::unique_ptr<ISystem> system = factory();
        std::string name(system->name());
        const std::size_t entryIndex = m_entries.size();
        m_entries.push_back({std::move(system), systemType, std::move(name), std::move(access), {}, 0, Slot::Regular});
        try {
            m_entryByType.emplace(systemType, entryIndex);
            ScheduleRegistration registration(*this, entryIndex);
            m_entries[entryIndex].system->init(registration);
            return registration;
        } catch (...) {
            m_entryByType.erase(systemType);
            m_entries.pop_back();
            throw;
        }
    }

    void Schedule::setPriority(const std::size_t entryIndex, const int value) {
        checkedEntry(entryIndex).priority = value;
    }

    void Schedule::addAccess(const std::size_t entryIndex, const std::span<const ComponentTypeId> written, const std::span<const ComponentTypeId> readOnly) {
        ScheduleAccess& access = checkedEntry(entryIndex).access;
        const ScheduleAccess added(*m_componentRegistry, written, readOnly);
        for (std::size_t i = 0; i < added.m_writeMask.size(); i++) {
            ScheduleAccess::orWord(access.m_writeMask, i, added.m_writeMask[i]);
        }
        for (std::size_t i = 0; i < added.m_readMask.size(); i++) {
            ScheduleAccess::orWord(access.m_readMask, i, added.m_readMask[i]);
        }
    }

    void Schedule::setSlot(const std::size_t entryIndex, const Slot value) {
        ScheduleEntry& entry = checkedEntry(entryIndex);

        if (value == Slot::Terminal) {
            for (std::size_t i = 0; i < m_entries.size(); i++) {
                TE_CHECK(i == entryIndex || m_entries[i].slot != Slot::Terminal, "Only one terminal system can be registered");
            }
        }

        entry.slot = value;
    }

    void Schedule::addOrder(const std::size_t entryIndex, const std::type_index systemType, const Order order) {
        checkedEntry(entryIndex).orderConstraints.push_back({systemType, order});
    }

    void Schedule::addEventHandler(const std::size_t entryIndex, EventHandler handler) {
        checkedEntry(entryIndex).eventHandlers.push_back(std::move(handler));
    }

    void Schedule::addInputHandler(const std::size_t entryIndex, InputHandler handler) {
        checkedEntry(entryIndex).inputHandlers.push_back(std::move(handler));
    }

    ScheduleEntry& Schedule::checkedEntry(const std::size_t entryIndex) {
        TE_CHECK(!m_frozen, "Cannot modify schedule after freezing");
        TE_CHECK(entryIndex < m_entries.size(), "Invalid schedule entry index");
        return m_entries.at(entryIndex);
    }
}
