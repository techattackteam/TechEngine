#pragma once

#include <cstdint>
#include "GLFW.hpp"

class FrameBuffer {
private:
    uint32_t id;

public:
    uint32_t depthMap;

    FrameBuffer() = default;

    ~FrameBuffer();

    void bind();

    void unBind();

    void createDepthTexture(uint32_t width, uint32_t height);

    void clear();

    void bindShadowMapTexture();

    void init(uint32_t i);
};


