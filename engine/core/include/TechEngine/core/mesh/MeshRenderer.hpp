#pragma once

#include <TechEngine/core/resources/AssetRef.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace TechEngine {
    struct Material;
    struct Mesh;

    struct MaterialSlot {
        std::string name;
        AssetRef<Material> material;

        bool operator==(const MaterialSlot&) const = default;

        template<typename Archive>
        friend void visit(Archive& archive, MaterialSlot& value) {
            archive.field("name", value.name);
            archive.field("material", value.material);
        }
    };

    struct MeshRenderer {
        static constexpr std::string_view tag = "TechEngine::MeshRenderer";

        AssetRef<Mesh> mesh;
        std::vector<MaterialSlot> slots;

        // A slot that takes its mesh's default gets the id with an empty handle, so the caller must acquire it.
        void rebuildSlots(const Mesh& source, std::vector<std::string>& dropped);

        template<typename Archive>
        friend void visit(Archive& archive, MeshRenderer& value) {
            archive.field("mesh", value.mesh);
            archive.field("slots", value.slots);
        }
    };
}
