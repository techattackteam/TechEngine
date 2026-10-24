#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>

namespace TechEngine {
    struct SceneAssetEntity {
        Entity entity;
        std::size_t childCount = 0;
    };
}
