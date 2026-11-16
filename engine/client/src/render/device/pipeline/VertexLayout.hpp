#pragma once

#include <cstdint>
#include <span>

namespace TechEngine {
    inline constexpr std::uint32_t MAX_VERTEX_BINDINGS = 4;

    enum class VertexFormat : std::uint8_t { Float2, Float3, Float4 };

    struct VertexAttribute {
        std::uint32_t location = 0;
        std::uint32_t binding = 0;
        VertexFormat format = VertexFormat::Float3;
        std::uint32_t offset = 0;
    };

    struct VertexBinding {
        std::uint32_t binding = 0;
        std::uint32_t stride = 0;
    };

    struct VertexLayout {
        std::span<const VertexAttribute> attributes;
        std::span<const VertexBinding> bindings;
    };
}
