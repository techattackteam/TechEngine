#include <TechEngine/base/diagnostics/Assert.hpp>
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
        if (tag.empty()) {
            TE_CHECK(false, "An event tag must not be empty");
            return {};
        }

        const EventTypeId id{StringId{tag}};
        if (!id.valid()) {
            TE_CHECK(false, "Event tag hashes to the invalid id: {0}", tag);
            return {};
        }

        const auto existing = m_indexById.find(id);
        if (existing != m_indexById.end()) {
            const std::string_view registered = m_eventRecords[existing->second].tag;
            if (registered == tag) {
                TE_CHECK(false, "Event tag already registered: {0}", tag);
            } else {
                TE_CHECK(false, "StringId collision — {0} and {1} share an id", tag, registered);
            }
            return {};
        }

        const std::uint32_t streamIndex = static_cast<std::uint32_t>(m_eventRecords.size());
        m_eventRecords.emplace_back(id, std::string{tag}, streamIndex, size, alignment, wire);
        m_indexById.emplace(id, streamIndex);
        return id;
    }
}
