#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <concepts>
#include <format>
#include <string_view>

namespace TechEngine {
    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;

    using IVec2 = glm::ivec2;
    using IVec3 = glm::ivec3;
    using IVec4 = glm::ivec4;

    using UVec2 = glm::uvec2;
    using UVec3 = glm::uvec3;
    using UVec4 = glm::uvec4;

    using Mat2 = glm::mat2;
    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;

    using Mat2x3 = glm::mat2x3;
    using Mat2x4 = glm::mat2x4;

    using Mat3x2 = glm::mat3x2;
    using Mat3x4 = glm::mat3x4;

    using Mat4x2 = glm::mat4x2;
    using Mat4x3 = glm::mat4x3;

    using Quat = glm::quat;

    namespace internal {
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
}

template<glm::length_t L, typename T, glm::qualifier Q>
struct std::formatter<glm::vec<L, T, Q>> {
    constexpr auto parse(std::format_parse_context& context) {
        return m_element.parse(context);
    }

    auto format(const glm::vec<L, T, Q>& value, std::format_context& context) const {
        auto out = std::format_to(context.out(), "{0}vec{1}(", TechEngine::internal::mathTypePrefix<T>(), L);

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
        auto out = std::format_to(context.out(), "{0}mat", TechEngine::internal::mathTypePrefix<T>());
        if constexpr (C == R) {
            out = std::format_to(out, "{1}(", TechEngine::internal::mathTypePrefix<T>(), C);
        } else {
            out = std::format_to(out, "{1}x{2}(", TechEngine::internal::mathTypePrefix<T>(), C, R);
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
        auto out = std::format_to(context.out(), "{0}quat(", TechEngine::internal::mathTypePrefix<T>());
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
