#pragma once

#include <TechEngine/base/stringid/StringId.hpp>

#include <compare>
#include <cstddef>
#include <functional>

namespace TechEngine {
    struct EventTypeId {
        constexpr EventTypeId() = default;

        constexpr explicit EventTypeId(StringId tag) : m_value{tag} {
        }

        constexpr StringId value() const {
            return m_value;
        }

        constexpr bool valid() const {
            return m_value != StringId{};
        }

        bool operator==(const EventTypeId&) const = default;
        auto operator<=>(const EventTypeId&) const = default;

    private:
        StringId m_value{};
    };

    namespace detail {
        template<typename T>
        inline EventTypeId g_eventTypeSlot{};
    }

    template<typename T>
    EventTypeId eventTypeId() {
        return detail::g_eventTypeSlot<T>;
    }
}

template<>
struct std::hash<TechEngine::EventTypeId> {
    std::size_t operator()(TechEngine::EventTypeId id) const noexcept {
        return std::hash<TechEngine::StringId>{}(id.value());
    }
};
