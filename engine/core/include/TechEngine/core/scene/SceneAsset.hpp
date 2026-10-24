#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    class ResourceCache;
    class Scene;

    inline constexpr BlobKind SCENE_KIND{0x43534554, 1};

    inline constexpr StringId SCENE_TYPE_TAG{"TechEngine.Scene"};

    // The cache keys a payload's type tag by its C++ type, so a scene needs a type of its own. A
    // bare byte vector here would claim that tag for every resource that keeps raw bytes.
    struct SceneBytes {
        std::vector<std::byte> bytes;

        static std::error_code loadPayload(std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies);
    };

    struct SceneAsset {
        ResourceId id;

        void write(const Scene& scene, std::vector<std::byte>& out) const;

        std::error_code read(std::span<const std::byte> bytes, Scene& scene);

        std::error_code load(ResourceCache& cache, Scene& scene, ResourceHandle<SceneBytes>& out) const;
    };
}
