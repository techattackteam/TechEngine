#pragma once

#include <TechEngine/base/stringid/StringId.hpp>

#include <format>

template<>
struct std::formatter<TechEngine::StringId> {
    constexpr auto parse(std::format_parse_context& context) {
        auto it = context.begin();
        if (it != context.end() && *it != '}') {
            throw std::format_error("StringId takes no format spec");
        }
        return it;
    }

    auto format(TechEngine::StringId id, std::format_context& context) const {
        return std::format_to(context.out(), "{0:#018x}", id.value());
    }
};
