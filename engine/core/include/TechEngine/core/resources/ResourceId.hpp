#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace TechEngine {
    struct ResourceId {
        constexpr ResourceId() = default;

        constexpr explicit ResourceId(const std::uint64_t value) : m_value(value) {
        }

        constexpr std::uint64_t value() const {
            return m_value;
        }

        constexpr bool valid() const {
            return m_value != 0;
        }

        bool operator==(const ResourceId&) const = default;
        auto operator<=>(const ResourceId&) const = default;

    private:
        std::uint64_t m_value = 0;
    };
}

template<>
struct std::hash<TechEngine::ResourceId> {
    std::size_t operator()(TechEngine::ResourceId id) const noexcept {
        return std::hash<std::uint64_t>{}(id.value());
    }
};
