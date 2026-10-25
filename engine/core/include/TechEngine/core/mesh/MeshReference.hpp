#pragma once

#include <TechEngine/core/resources/AssetRef.hpp>

#include <string_view>

namespace TechEngine {
    struct Mesh;

    struct MeshReference {
        static constexpr std::string_view tag = "TechEngine::MeshReference";

        AssetRef<Mesh> mesh;

        template<typename Archive>
        friend void visit(Archive& archive, MeshReference& value) {
            archive.field(value.mesh);
        }
    };
}
