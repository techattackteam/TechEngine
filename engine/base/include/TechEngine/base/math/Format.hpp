#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <concepts>
#include <format>
#include <string_view>

namespace TechEngine::detail {
    template<typename T>
    constexpr std::string_view mathTypePrefix() {
        if constexpr (std::same_as<T, double>) {
            return "d";
        } else if constexpr (std::same_as<T, int>) {
            return "i";
        } else if constexpr (std::same_as<T, unsigned int>) {
            return "u";
        } else if constexpr (std::same_as<T, bool>) {
            return "b";
        } else {
            return "";
        }
    }
}

template<glm::length_t L, typename T, glm::qualifier Q>
struct std::formatter<glm::vec<L, T, Q>> {
    constexpr auto parse(std::format_parse_context& context) {
        return m_element.parse(context);
    }

    auto format(const glm::vec<L, T, Q>& value, std::format_context& context) const {
        auto out = std::format_to(context.out(), "{0}vec{1}(", TechEngine::detail::mathTypePrefix<T>(), L);

        for (glm::length_t i = 0; i < L; i++) {
            if (i > 0) {
                out = std::format_to(out, ", ");
            }
            context.advance_to(out);
            out = m_element.format(value[i], context);
        }

        return std::format_to(out, ")");
    }

private:
    std::formatter<T> m_element;
};

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct std::formatter<glm::mat<C, R, T, Q>> {
    constexpr auto parse(std::format_parse_context& context) {
        return m_element.parse(context);
    }

    auto format(const glm::mat<C, R, T, Q>& value, std::format_context& context) const {
        // TODO(S3-T2): "<prefix>mat<C>((c0…), (c1…))" — square is "mat4", else "mat3x4";
        auto out = std::format_to(context.out(), "{0}mat", TechEngine::detail::mathTypePrefix<T>());
        if constexpr (C == R) {
            out = std::format_to(out, "{1}(", TechEngine::detail::mathTypePrefix<T>(), C);
        } else {
            out = std::format_to(out, "{1}x{2}(", TechEngine::detail::mathTypePrefix<T>(), C, R);
        }
        for (glm::length_t i = 0; i < C; i++) {
            if (i > 0) {
                out = std::format_to(out, ", ");
            }

            out = std::format_to(out, "(");
            for (glm::length_t j = 0; j < R; j++) {
                if (j > 0) {
                    out = std::format_to(out, ", ");
                }
                context.advance_to(out);
                out = m_element.format(value[i][j], context);
            }
            out = std::format_to(out, ")");
        }

        return std::format_to(out, ")");
    }

private:
    std::formatter<T> m_element;
};

template<typename T, glm::qualifier Q>
struct std::formatter<glm::qua<T, Q>> {
    constexpr auto parse(std::format_parse_context& context) {
        return m_element.parse(context);
    }

    auto format(const glm::qua<T, Q>& value, std::format_context& context) const {
        // TODO(S3-T2): "<prefix>quat(x, y, z, w)" — storage order, not glm::to_string's wxyz
        auto out = std::format_to(context.out(), "{0}quat(", TechEngine::detail::mathTypePrefix<T>());
        const T components[4]{value.x, value.y, value.z, value.w};

        for (glm::length_t i = 0; i < 4; i++) {
            if (i > 0) {
                out = std::format_to(out, ", ");
            }
            context.advance_to(out);
            out = m_element.format(components[i], context);
        }

        return std::format_to(out, ")");
    }

private:
    std::formatter<T> m_element;
};
