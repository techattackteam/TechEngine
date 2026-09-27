#pragma once

#include <cstdint>
#include <string_view>

namespace TechEngine {
    struct EntitySpawned {
        static constexpr std::string_view tag = "TechEngine::EntitySpawned";

        std::uint32_t ordinal;
    };
}
