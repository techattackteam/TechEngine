#pragma once

#include <TechEngine/core/resources/AssetRef.hpp>

#include <string_view>

namespace TechEngine {
    struct Mesh;

    struct MeshRenderer {
        static constexpr std::string_view tag = "TechEngine::MeshRenderer";

        AssetRef<Mesh> mesh;

        template<typename Archive>
        friend void visit(Archive& archive, MeshRenderer& value) {
            archive.field(value.mesh);
        }
    };
}
