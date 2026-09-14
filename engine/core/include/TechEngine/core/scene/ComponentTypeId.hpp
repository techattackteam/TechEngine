#pragma once

#include <TechEngine/base/stringid/StringId.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>

namespace TechEngine {
    struct ComponentTypeId {
        constexpr ComponentTypeId() = default;

        constexpr explicit ComponentTypeId(const StringId tag) : m_value{tag} {
        }

        constexpr StringId stringId() const {
            return m_value;
        }

        constexpr bool valid() const {
            return m_value != StringId{};
        }

        bool operator==(const ComponentTypeId&) const = default;
        auto operator<=>(const ComponentTypeId&) const = default;

    private:
        StringId m_value{};
    };

    struct ComponentDenseId {
        static constexpr std::uint32_t VALUE_COUNT = static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max()) + 1U;

        constexpr ComponentDenseId() = default;

        constexpr explicit ComponentDenseId(const std::uint16_t value) : m_value{value} {
        }

        constexpr std::uint16_t value() const {
            return m_value;
        }

        bool operator==(const ComponentDenseId&) const = default;
        auto operator<=>(const ComponentDenseId&) const = default;

    private:
        std::uint16_t m_value = 0;
    };

    namespace internal {
        template<typename T>
        inline ComponentTypeId g_componentTypeSlot{};
    }

    template<typename T>
    ComponentTypeId componentTypeId() {
        return internal::g_componentTypeSlot<T>;
    }
}

template<>
struct std::hash<TechEngine::ComponentTypeId> {
    std::size_t operator()(TechEngine::ComponentTypeId id) const noexcept {
        return std::hash<TechEngine::StringId>{}(id.stringId());
    }
};

template<>
struct std::hash<TechEngine::ComponentDenseId> {
    std::size_t operator()(TechEngine::ComponentDenseId id) const noexcept {
        return std::hash<std::uint16_t>{}(id.value());
    }
};
