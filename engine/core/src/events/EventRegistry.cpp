
#include <TechEngine/core/events/EventRegistry.hpp>

namespace TechEngine {
    const EventTypeRecord* EventRegistry::find(EventTypeId id) const {
        const auto it = m_indexById.find(id);
        if (it != m_indexById.end()) {
            return &m_eventRecords[it->second];
        }
        return nullptr;
    }

    std::string_view EventRegistry::tagOf(EventTypeId id) const {
        const auto it = m_indexById.find(id);
        if (it != m_indexById.end()) {
            return m_eventRecords[it->second].tag;
        }
        return {};
    }
    std::size_t EventRegistry::typeCount() const {
        return m_eventRecords.size();
    }

    EventTypeId EventRegistry::registerType(std::string_view tag, std::uint32_t size, std::uint32_t alignment, EventWire wire) {
        m_eventRecords.push_back({EventTypeId{StringId(tag)}, std::string(tag), static_cast<std::uint32_t>(m_eventRecords.size()), size, alignment, wire});
        return m_eventRecords.back().id;
    }
}