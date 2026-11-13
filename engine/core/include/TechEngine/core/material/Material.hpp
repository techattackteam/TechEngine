#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <system_error>
#include <vector>

namespace TechEngine {
    struct Texture;

    inline constexpr BlobKind MATERIAL_KIND{0x414D4554, 1};

    inline constexpr StringId MATERIAL_TYPE_TAG{"TechEngine.Material"};

    enum class MaterialSurface : std::uint8_t {
        Standard = 0,
    };

    enum class MaterialShadingModel : std::uint8_t {
        Lit = 0,
        Unlit,
    };

    enum class MaterialAlphaMode : std::uint8_t {
        Opaque = 0,
        Cutout,
        Blended,
    };

    struct Material {
        ResourceId id;
        ImportBlock importBlock;
        MaterialSurface surface = MaterialSurface::Standard;
        MaterialShadingModel shadingModel = MaterialShadingModel::Lit;
        MaterialAlphaMode alphaMode = MaterialAlphaMode::Opaque;
        float alphaCutoff = 0.5f;
        bool doubleSided = false;
        Vec4 baseColorFactor = Vec4(1.0f);
        AssetRef<Texture> baseColorTexture;
        float occlusionStrength = 1.0f;
        float roughnessFactor = 1.0f;
        float metallicFactor = 1.0f;
        // Occlusion in red, roughness in green, metallic in blue.
        AssetRef<Texture> occlusionRoughnessMetallicTexture;
        float normalScale = 1.0f;
        AssetRef<Texture> normalTexture;
        Vec3 emissiveFactor = Vec3(0.0f);
        AssetRef<Texture> emissiveTexture;

        void write(std::vector<std::byte>& out) const;

        std::error_code read(std::span<const std::byte> bytes);

        static std::error_code loadPayload(std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies);

        bool operator==(const Material&) const = default;
    };
}
