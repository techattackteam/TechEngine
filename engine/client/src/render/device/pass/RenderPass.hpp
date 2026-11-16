#pragma once

#include <render/device/RenderExtent.hpp>
#include <render/device/RenderHandle.hpp>

#include <array>
#include <cstdint>

namespace TechEngine {
    enum class LoadAction : std::uint8_t { Load, Clear, DontCare };

    enum class StoreAction : std::uint8_t { Store, DontCare };

    struct RenderPassDescription {
        RenderTargetHandle target;
        RenderExtent extent;
        LoadAction colorLoad = LoadAction::Clear;
        std::array<float, 4> clearColor{};
        StoreAction colorStore = StoreAction::Store;
        LoadAction depthLoad = LoadAction::Clear;
        float clearDepth = 1.0F;
        StoreAction depthStore = StoreAction::DontCare;
    };
}
