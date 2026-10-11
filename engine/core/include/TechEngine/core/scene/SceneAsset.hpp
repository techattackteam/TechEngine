#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    class Scene;

    inline constexpr BlobKind SCENE_KIND{0x43534554, 1};

    void writeSceneAsset(const Scene& scene, ResourceId id, std::vector<std::byte>& out);

    std::error_code readSceneAsset(std::span<const std::byte> bytes, Scene& scene, ResourceId& id);
}
