#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace TechEngineTests {
    template<typename Event>
    static std::vector<std::uint32_t> amountsOf(const std::span<const Event> events) {
        std::vector<std::uint32_t> amounts;
        for (const Event& event: events) {
            amounts.push_back(event.amount);
        }
        return amounts;
    }
}
