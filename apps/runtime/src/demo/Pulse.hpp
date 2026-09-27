#pragma once

#include <cstdint>
#include <string_view>

namespace TechEngine {
    struct Pulse {
        static constexpr std::string_view tag = "TechEngine::Pulse";

        std::uint32_t sequence;
    };
}
