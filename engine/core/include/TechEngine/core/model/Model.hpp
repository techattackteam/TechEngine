#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace TechEngine {
    inline constexpr BlobKind MODEL_KIND{0x4F4D4554, 1};

    inline constexpr StringId MODEL_TYPE_TAG{"TechEngine.Model"};

    inline constexpr std::uint32_t MODEL_NO_PARENT = 0xFFFFFFFF;

    struct ModelNode {
        std::string name;
        std::uint32_t parent = MODEL_NO_PARENT;
        TransformValues local;
        ResourceId mesh;
    };

    struct Model {
        ResourceId id;
        ImportBlock importBlock;
        std::vector<ModelNode> nodes;

        void write(std::vector<std::byte>& out) const;

        std::error_code read(std::span<const std::byte> bytes);
    };
}
