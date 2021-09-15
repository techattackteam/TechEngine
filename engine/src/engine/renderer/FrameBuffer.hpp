#pragma once

#include <cstdint>
#include "GLFW.hpp"

class FrameBuffer {
private:
    uint32_t id;

    uint32_t depthMap;
public:
    explicit FrameBuffer(uint32_t id);

    ~FrameBuffer();

    void bind();

    void unBind();

    void createDepthTexture(uint32_t width, uint32_t height);

    void clear();

    void bindShadowMapTexture();
};


