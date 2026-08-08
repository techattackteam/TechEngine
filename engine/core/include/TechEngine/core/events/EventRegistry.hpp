#pragma once

#include <TechEngine/core/events/EventTypeId.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    enum class EventWire : std::uint8_t {
        Local,
        Replicated,
    };

    struct EventTypeRecord {
        EventTypeId id;
        std::string tag;
        std::uint32_t streamIndex{0};
        std::uint32_t size{0};
        std::uint32_t alignment{0};
        EventWire wire{EventWire::Local};
    };

    class EventRegistry {
    private:
        std::vector<EventTypeRecord> m_eventRecords;
        std::unordered_map<EventTypeId, std::uint32_t> m_indexById;
        bool m_sealed = false;

    public:
        EventRegistry() = default;

        EventRegistry(const EventRegistry&) = delete;

        EventRegistry& operator=(const EventRegistry&) = delete;

        template<typename T>
        EventTypeId registerEvent(std::string_view tag, EventWire wire = EventWire::Local) {
            static_assert(std::is_trivially_copyable_v<T>, "An event payload must be trivially copyable");

            const EventTypeId id = registerType(tag, static_cast<std::uint32_t>(sizeof(T)), static_cast<std::uint32_t>(alignof(T)), wire);
            if (id.valid()) {
                detail::g_eventTypeSlot<T> = id;
            }
            return id;
        }

        const EventTypeRecord* find(EventTypeId id) const;

        std::span<const EventTypeRecord> records() const;

        void seal();

        bool sealed() const;

        std::string_view tagOf(EventTypeId id) const;

        std::size_t typeCount() const;

    private:
        EventTypeId registerType(std::string_view tag, std::uint32_t size, std::uint32_t alignment, EventWire wire);
    };
}
