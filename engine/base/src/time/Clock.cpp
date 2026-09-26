#include <TechEngine/base/time/Clock.hpp>

namespace TechEngine {
    Clock::Clock() {
        m_start = std::chrono::steady_clock::now();
    }

    Clock::TimePoint Clock::now() const {
        return std::chrono::steady_clock::now();
    }

    double Clock::totalTime() const {
        return std::chrono::duration_cast<std::chrono::duration<double>>(now() - m_start).count();
    }

    Clock::WallPoint Clock::wallClock() const {
        return std::chrono::system_clock::now();
    }

    std::uint64_t Clock::tick() const {
        return m_tick.load(std::memory_order_relaxed);
    }

    void Clock::advanceTick() {
        m_tick.fetch_add(1, std::memory_order_relaxed);
    }
}
