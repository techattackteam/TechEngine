#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    inline constexpr BlobKind MESH_KIND{0x534D4554, 1};

    inline constexpr StringId MESH_TYPE_TAG{"TechEngine.Mesh"};

    struct Mesh {
        ResourceId id;
        std::vector<Vec3> positions;
        std::vector<std::uint32_t> indices;

        void write(std::vector<std::byte>& out) const;

        std::error_code read(std::span<const std::byte> bytes);

        static std::error_code loadPayload(std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies);
    };
}
