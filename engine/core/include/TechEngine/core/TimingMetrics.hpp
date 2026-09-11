#pragma once

#include <TechEngine/base/time/Clock.hpp>

#include <cstdint>
#include <optional>

namespace TechEngine {
    struct SimulationTiming {
        std::uint64_t tick = 0;
        std::uint64_t timeline = 0;
        double ticksPerSecond = 0.0;
        double tickWorkDuration = 0.0;
        Clock::TimePoint sampledAt{};
    };

    struct RenderTiming {
        std::uint64_t frame = 0;
        double framesPerSecond = 0.0;
        double frameInterval = 0.0;
        double renderWorkDuration = 0.0;
        Clock::TimePoint sampledAt{};
    };

    struct TimingMetrics {
        SimulationTiming simulation;
        std::optional<RenderTiming> render;
    };
}
