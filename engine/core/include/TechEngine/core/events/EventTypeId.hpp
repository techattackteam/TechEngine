#pragma once

#include <TechEngine/base/stringid/StringId.hpp>

#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <string_view>
#include <type_traits>

namespace TechEngine {
    template<typename T>
    concept EventType = std::is_trivially_copyable_v<T> && requires {
        { T::tag } -> std::convertible_to<std::string_view>;
    };

    struct EventTypeId {
        constexpr EventTypeId() = default;

        constexpr explicit EventTypeId(StringId tag) : m_value(tag) {
        }

        constexpr StringId stringId() const {
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

    namespace internal {
        template<typename T>
        inline EventTypeId g_eventTypeSlot{};
    }

    template<typename T>
    EventTypeId eventTypeId() {
        return internal::g_eventTypeSlot<T>;
    }
}

template<>
struct std::hash<TechEngine::EventTypeId> {
    std::size_t operator()(TechEngine::EventTypeId id) const noexcept {
        return std::hash<TechEngine::StringId>{}(id.stringId());
    }
};
