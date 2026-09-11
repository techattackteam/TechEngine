#pragma once

#include <cstdint>

namespace TechEngine {
    class RateCounter {
    private:
        double m_elapsed = 0.0;
        std::uint64_t m_count = 0;
        double m_rate = 0.0;
        std::uint64_t m_sampleIndex = 0;

    public:
        void advance(double elapsed, std::uint64_t completed);
        double rate() const;
        std::uint64_t sampleIndex() const;
    };
}
