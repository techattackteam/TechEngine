#pragma once

#include <cstdint>
#include "GLFW.hpp"

namespace TechEngine {
    class FrameBuffer {
    private:
        uint32_t id = 1;

    public:
        uint32_t depthTexture = 0;
        uint32_t colorTexture = 0;
        uint32_t width = 0;
        uint32_t height = 0;

        FrameBuffer() = default;

        ~FrameBuffer();

        void init(uint32_t id, uint32_t width, uint32_t height);

        void bind();

        void unBind();

        void resize(uint32_t width, uint32_t height);

        uint32_t getColorAttachmentRenderer();

        void clear();

        void bindShadowMapTexture();

        void attachDepthTexture();

        void attachDepthTexture(uint32_t width, uint32_t height);

        void attachColorTexture();

        void attachColorTexture(uint32_t width, uint32_t height);

        uint32_t getID();
    };
}
