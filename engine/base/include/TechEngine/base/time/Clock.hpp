#pragma once

#include <chrono>
#include <cstdint>

namespace TechEngine {
    class Clock {
    public:
        using TimePoint = std::chrono::steady_clock::time_point;
        using WallPoint = std::chrono::system_clock::time_point;

        Clock();

        TimePoint now() const;

        double totalTime() const;

        WallPoint wallClock() const;

        std::uint64_t frame() const;

        void advanceFrame();

    private:
        TimePoint m_start;
        std::uint64_t m_frame{0};
    };
}
