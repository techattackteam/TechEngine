#include <TechEngine/base/diagnostics/Assert.hpp>

#include <render/device/material/Material.hpp>

namespace TechEngine {
    void GpuMaterial::setTextureHandle(const std::size_t input, const std::uint64_t bindlessHandle) {
        TE_CHECK(input < MATERIAL_TEXTURE_INPUT_COUNT, "GpuMaterial::setTextureHandle called with input {0} of {1}", input, MATERIAL_TEXTURE_INPUT_COUNT);
        if (input >= MATERIAL_TEXTURE_INPUT_COUNT) {
            return;
        }
        textureHandles[input * 2] = static_cast<std::uint32_t>(bindlessHandle);
        textureHandles[input * 2 + 1] = static_cast<std::uint32_t>(bindlessHandle >> 32U);
    }
}
