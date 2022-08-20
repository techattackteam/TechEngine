#include "FrameBuffer.hpp"
#include "ErrorCatcher.hpp"

void FrameBuffer::init(uint32_t id) {
    this->id = id;
    GlCall(glGenFramebuffers(1, &this->id));
}


FrameBuffer::~FrameBuffer() {
    GlCall(glDeleteFramebuffers(1, &this->id));
}

void FrameBuffer::bind() {
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, this->id));
}

void FrameBuffer::unBind() {
    GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void FrameBuffer::createDepthTexture(uint32_t width, uint32_t height) {
    GlCall(glGenTextures(1, &depthMap));
    GlCall(glBindTexture(GL_TEXTURE_2D, depthMap));
    GlCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));

    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

    bind();
    GlCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0));
    GlCall(glDrawBuffer(GL_NONE));
    GlCall(glReadBuffer(GL_NONE));
    unBind();
}

void FrameBuffer::clear() {
    GlCall(glClear(GL_DEPTH_BUFFER_BIT));
}

void FrameBuffer::bindShadowMapTexture() {
    GlCall(glActiveTexture(GL_TEXTURE0));
    GlCall(glBindTexture(GL_TEXTURE_2D, depthMap));
}

