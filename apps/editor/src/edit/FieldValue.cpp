#include <edit/FieldValue.hpp>

#include <format>
#include <type_traits>

namespace TechEngine {
    std::string describeFieldValue(const FieldValue& value) {
        return std::visit(
            [](const auto& held) -> std::string {
                using T = std::decay_t<decltype(held)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    return std::format("\"{0}\"", held);
                } else if constexpr (std::is_same_v<T, std::int8_t> || std::is_same_v<T, std::uint8_t>) {
                    // std::format would print a one-byte integer as a character.
                    return std::format("{0}", static_cast<int>(held));
                } else {
                    return std::format("{0}", held);
                }
            },
            value);
    }
}
