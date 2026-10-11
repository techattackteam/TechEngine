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

    struct SceneAsset {
        ResourceId id;

        void write(const Scene& scene, std::vector<std::byte>& out) const;

        std::error_code read(std::span<const std::byte> bytes, Scene& scene);
    };
}
