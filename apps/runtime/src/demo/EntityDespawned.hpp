#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <string_view>

namespace TechEngine {
    struct EntityDespawned {
        static constexpr std::string_view tag = "TechEngine::EntityDespawned";

        Entity entity;
    };
}
