#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>

namespace TechEngine {
    struct Entity {
        static constexpr std::uint32_t NULL_INDEX = std::numeric_limits<std::uint32_t>::max();

        std::uint32_t index = NULL_INDEX;
        std::uint32_t generation = 0;

        constexpr bool valid() const {
            return index != NULL_INDEX;
        }

        bool operator==(const Entity&) const = default;
        auto operator<=>(const Entity&) const = default;
    };
}

template<>
struct std::hash<TechEngine::Entity> {
    std::size_t operator()(TechEngine::Entity entity) const noexcept {
        const std::size_t index = std::hash<std::uint32_t>{}(entity.index);
        const std::size_t generation = std::hash<std::uint32_t>{}(entity.generation);
        return index ^ (generation + 0x9e3779b9U + (index << 6U) + (index >> 2U));
    }
};

template<>
struct std::formatter<TechEngine::Entity> {
    constexpr auto parse(std::format_parse_context& context) {
        auto it = context.begin();
        if (it != context.end() && *it != '}') {
            throw std::format_error("Entity takes no format spec");
        }
        return it;
    }

    auto format(TechEngine::Entity entity, std::format_context& context) const {
        return std::format_to(context.out(), "{0}:{1}", entity.index, entity.generation);
    }
};
