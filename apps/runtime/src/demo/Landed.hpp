#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <string_view>

namespace TechEngine {
    struct Landed {
        static constexpr std::string_view tag = "TechEngine::Landed";

        Entity entity;
    };
}
