#pragma once

#include <render/device/RenderExtent.hpp>
#include <render/device/RenderHandle.hpp>

namespace TechEngine {
    struct RenderTargetDescription {
        TextureHandle color;
        TextureHandle depth;
    };

    struct GpuRenderTarget {
        unsigned int framebuffer = 0;
        RenderExtent extent;
        TextureHandle color;
        TextureHandle depth;
    };
}
