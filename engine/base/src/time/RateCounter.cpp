#include <TechEngine/base/time/RateCounter.hpp>

#include <cmath>

namespace TechEngine {
    void RateCounter::advance(double elapsed, std::uint64_t completed) {
        if (!std::isfinite(elapsed) || elapsed < 0.0) {
            return;
        }
        m_elapsed += elapsed;
        m_count += completed;
        if (m_elapsed >= 1.0) {
            m_rate = static_cast<double>(m_count) / m_elapsed;
            m_sampleIndex++;
            m_elapsed = 0.0;
            m_count = 0;
        }
    }

    double RateCounter::rate() const {
        return m_rate;
    }
    std::uint64_t RateCounter::sampleIndex() const {
        return m_sampleIndex;
    }
}
