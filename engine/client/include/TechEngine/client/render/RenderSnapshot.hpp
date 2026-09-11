#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <array>
#include <cstdint>

namespace TechEngine {
    struct RenderSnapshot {
        std::array<float, 4> clearColor{};
        bool drawTriangle = false;
        std::uint64_t tick = 0;
        Clock::TimePoint tickTime{};
        double fixedDeltaTime = 1.0 / 60.0;
        std::uint64_t timeline = 0;
        InputState input;
    };
}
