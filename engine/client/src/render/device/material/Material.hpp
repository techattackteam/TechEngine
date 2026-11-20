#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace TechEngine {
    inline constexpr std::size_t MATERIAL_TEXTURE_INPUT_COUNT = 4;

    struct GpuMaterial {
        Vec4 albedo = Vec4(0.0f);
        Vec3 emissiveFactor = Vec3(0.0f);
        float alphaCutoff = 0.0f;
        float occlusionStrength = 0.0f;
        float roughnessFactor = 0.0f;
        float metallicFactor = 0.0f;
        float normalScale = 0.0f;
        // Base color, packed ORM, normal and emissive. Each handle is two words, low word first, the order sampler2D(uvec2) reads.
        std::array<std::uint32_t, 2 * MATERIAL_TEXTURE_INPUT_COUNT> textureHandles{};
        std::uint32_t shadingModel = 0;
        std::array<std::uint32_t, 3> padding{};

        void setTextureHandle(std::size_t input, std::uint64_t bindlessHandle);

        bool operator==(const GpuMaterial&) const = default;
    };

    static_assert(sizeof(GpuMaterial) == 96, "a material record is 96 bytes in the shader's std430 buffer");
    static_assert(offsetof(GpuMaterial, textureHandles) == 48, "the texture handles start at byte 48 of a record");
    static_assert(offsetof(GpuMaterial, shadingModel) == 80, "the shading model sits at byte 80 of a record");
}
