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
        TE_CHECK(!m_frozen, "Cannot modify schedule after freezing");
        TE_CHECK(entryIndex < m_entries.size(), "Invalid schedule entry index");
        m_entries.at(entryIndex).priority = value;
    }

    void Schedule::addAccess(const std::size_t entryIndex, const std::span<const ComponentTypeId> written, const std::span<const ComponentTypeId> readOnly) {
        TE_CHECK(!m_frozen, "Cannot modify schedule after freezing");
        TE_CHECK(entryIndex < m_entries.size(), "Invalid schedule entry index");
        const ScheduleAccess added(*m_componentRegistry, written, readOnly);
        ScheduleAccess& access = m_entries.at(entryIndex).access;
        if (added.m_writeMask.size() > access.m_writeMask.size()) {
            access.m_writeMask.resize(added.m_writeMask.size(), 0);
        }
        if (added.m_readMask.size() > access.m_readMask.size()) {
            access.m_readMask.resize(added.m_readMask.size(), 0);
        }
        for (std::size_t i = 0; i < added.m_writeMask.size(); i++) {
            access.m_writeMask[i] |= added.m_writeMask[i];
        }
        for (std::size_t i = 0; i < added.m_readMask.size(); i++) {
            access.m_readMask[i] |= added.m_readMask[i];
        }
    }

    void Schedule::setSlot(const std::size_t entryIndex, const Slot value) {
        TE_CHECK(!m_frozen, "Cannot modify schedule after freezing");
        TE_CHECK(entryIndex < m_entries.size(), "Invalid schedule entry index");

        if (value == Slot::Terminal) {
            for (std::size_t i = 0; i < m_entries.size(); i++) {
                TE_CHECK(i == entryIndex || m_entries[i].slot != Slot::Terminal, "Only one terminal system can be registered");
            }
        }

        m_entries.at(entryIndex).slot = value;
    }

    void Schedule::addOrder(const std::size_t entryIndex, const std::type_index systemType, const Order order) {
        TE_CHECK(!m_frozen, "Cannot modify schedule after freezing");
        TE_CHECK(entryIndex < m_entries.size(), "Invalid schedule entry index");

        m_entries.at(entryIndex).orderConstraints.push_back({systemType, order});
    }
}
