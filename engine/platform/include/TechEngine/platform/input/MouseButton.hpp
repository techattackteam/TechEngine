#pragma once

#include <cstddef>
#include <cstdint>

namespace TechEngine {
    enum class MouseButton : std::uint8_t {
        Left,
        Right,
        Middle,
        Extra1,
        Extra2,
        Extra3,
        Extra4,
        Extra5,
        Unknown,
    };

    inline constexpr std::size_t MOUSE_BUTTON_COUNT = static_cast<std::size_t>(MouseButton::Unknown);
}
