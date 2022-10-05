#include "FrameBuffer.hpp"
#include "ErrorCatcher.hpp"
#include "RendererSettings.hpp"

namespace TechEngine {

    void FrameBuffer::init(uint32_t id, uint32_t width, uint32_t height) {
        this->width = width;
        this->height = height;
        if (this->id == id) {
            glDeleteFramebuffers(1, &id);
            glDeleteTextures(1, &colorTexture);
            glDeleteTextures(1, &depthMap);
        }
        this->id = id;
        GlCall(glGenFramebuffers(1, &this->id));
        glViewport(0, 0, width, height);
        GlCall(attachColorTexture(width, height));
    }


    FrameBuffer::~FrameBuffer() {
        GlCall(glDeleteFramebuffers(1, &this->id));
    }

    void FrameBuffer::bind() {
        GlCall(glBindFramebuffer(GL_FRAMEBUFFER, this->id));
        glViewport(0, 0, width, height);
    }

    void FrameBuffer::unBind() {
        GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }

    void FrameBuffer::resize(uint32_t width, uint32_t height) {
        if (width == 0 && height == 0) {
            return;
        }
        if (RendererSettings::targetId == id) {
            RendererSettings::resize(width, height);
        }
        init(id, width, height);
    }

    void FrameBuffer::attachColorTexture(uint32_t width, uint32_t height) {
        GlCall(glBindFramebuffer(GL_FRAMEBUFFER, this->id));
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        GlCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr));

        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        GlCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
        GlCall(glDrawBuffer(GL_COLOR_ATTACHMENT0));
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    uint32_t FrameBuffer::getColorAttachmentRenderer() {
        return colorTexture;
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

    int32_t FrameBuffer::getID() {
        return id;
    }
}