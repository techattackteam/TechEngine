#pragma once

#include <compare>
#include <cstdint>
#include <format>
#include <functional>
#include <string_view>

namespace TechEngine {
    namespace internal {
        inline constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        inline constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
    }

    struct StringId {
        constexpr StringId() = default;

        constexpr explicit StringId(std::string_view tag) {
            std::uint64_t hash = internal::FNV_OFFSET_BASIS;
            for (std::size_t i = 0; i < tag.size(); i++) {
                hash ^= static_cast<std::uint8_t>(tag[i]);
                hash *= internal::FNV_PRIME;
            }
            m_value = hash;
        }

        static constexpr StringId fromValue(std::uint64_t raw) {
            StringId id;
            id.m_value = raw;
            return id;
        }

        constexpr std::uint64_t value() const {
            return m_value;
        }

        bool operator==(const StringId&) const = default;
        auto operator<=>(const StringId&) const = default;

    private:
        std::uint64_t m_value = 0;
    };
}

template<>
struct std::hash<TechEngine::StringId> {
    std::size_t operator()(TechEngine::StringId id) const noexcept {
        return id.value();
    }
};

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
