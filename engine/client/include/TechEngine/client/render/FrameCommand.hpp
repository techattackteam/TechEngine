#pragma once

#include <array>
#include <cstdint>

namespace TechEngine {
    struct FrameCommand {
        std::array<float, 4> clearColor{};
        bool drawTriangle = false;
        std::uint64_t frameIndex = 0;
    };
}
