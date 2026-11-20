#pragma once

#include <cstdint>
#include <span>

namespace TechEngine {
    struct ImGuiViewTexture {
        std::uint64_t token = 0;
        std::uint64_t nativeTexture = 0;
    };

    // Every call runs on the render thread with the GL context current.
    class ImGuiHook {
    public:
        virtual ~ImGuiHook() = default;

        virtual void start() = 0;

        // The native names are good for this call only: a resize replaces a view's texture.
        virtual void draw(std::span<const ImGuiViewTexture> viewTextures) = 0;

        virtual void stop() = 0;
    };
}
