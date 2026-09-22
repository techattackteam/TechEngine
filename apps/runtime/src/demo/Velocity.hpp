#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <string_view>

namespace TechEngine {
    struct Velocity {
        static constexpr std::string_view tag = "TechEngine::Velocity";

        Vec3 linear = Vec3(0.0f);
    };
}
