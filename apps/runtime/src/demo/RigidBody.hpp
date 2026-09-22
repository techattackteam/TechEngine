#pragma once

#include <string_view>

namespace TechEngine {
    struct RigidBody {
        static constexpr std::string_view tag = "TechEngine::RigidBody";

        bool grounded = false;
    };
}
