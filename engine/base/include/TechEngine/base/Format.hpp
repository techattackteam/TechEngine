#pragma once

#include <concepts>
#include <format>
#include <string_view>
#include <type_traits>

namespace TechEngine {
    namespace detail {
        consteval bool hasAutomaticField(std::string_view text) {
            for (std::size_t i = 0; i < text.size(); ++i) {
                if (text[i] != '{') {
                    continue;
                }
                if (i + 1 < text.size() && text[i + 1] == '{') {
                    ++i;
                    continue;
                }

                const char next = i + 1 < text.size() ? text[i + 1] : '\0';
                if (next < '0' || next > '9') {
                    return true;
                }
            }
            return false;
        }
    }

    // std::format_string, narrowed to reject automatic indexing: "{0}" compiles, "{}" does
    // not. Positional is the house spelling (ADR-011 §1) because a diagnostic string that
    // names its slots survives being reordered or having an argument dropped.
    template<typename... Args>
    class PositionalFormatString {
    public:
        template<typename T>
            requires std::convertible_to<const T&, std::string_view>
        consteval PositionalFormatString(const T& text) : m_inner{text} {
            if (detail::hasAutomaticField(std::string_view{text})) {
                throw "TechEngine format strings index their arguments — write {0}, not {}";
            }
        }

        std::string_view get() const {
            return m_inner.get();
        }

    private:
        std::format_string<Args...> m_inner;
    };

    // type_identity_t keeps the format string out of deduction, so Args come from the
    // trailing pack alone — the same shape std::format_string uses.
    template<typename... Args>
    using PositionalFormat = PositionalFormatString<std::type_identity_t<Args>...>;
}
