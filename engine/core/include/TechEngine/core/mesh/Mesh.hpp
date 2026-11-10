#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace TechEngine {
    inline constexpr BlobKind MESH_KIND{0x534D4554, 2};

    inline constexpr StringId MESH_TYPE_TAG{"TechEngine.Mesh"};

    struct MeshSlot {
        std::string name;
        std::uint32_t firstIndex = 0;
        std::uint32_t indexCount = 0;
        ResourceId defaultMaterial;

        bool operator==(const MeshSlot&) const = default;
    };

    struct Mesh {
        ResourceId id;
        ImportBlock importBlock;
        std::vector<Vec3> positions;
        // Each optional stream is empty or holds one value per position.
        std::vector<Vec3> normals;
        // w is the bitangent's sign, +1 or -1.
        std::vector<Vec4> tangents;
        std::vector<Vec2> uv0;
        std::vector<std::uint32_t> indices;
        std::vector<MeshSlot> slots;

        void write(std::vector<std::byte>& out) const;

        std::error_code read(std::span<const std::byte> bytes);

        static std::error_code loadPayload(std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies);

        bool operator==(const Mesh&) const = default;
    };
}
