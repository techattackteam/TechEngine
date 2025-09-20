#include "FrameBuffer.hpp"
#include "ErrorCatcher.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    void FrameBuffer::init(uint32_t id, uint32_t width, uint32_t height) {
        this->width = width;
        this->height = height;
        if (this->id == id) {
            glDeleteFramebuffers(1, &id);
            glDeleteTextures(1, &colorTexture);
            glDeleteTextures(1, &depthTexture);
        }
        this->id = id;
        glGenFramebuffers(1, &this->id);
        glViewport(0, 0, width, height);
        bind();
        //Todo: Transfer this to the renderer init without for some reason crashing
        attachDepthTexture();
        attachColorTexture();
    }


    FrameBuffer::~FrameBuffer() {
        glDeleteFramebuffers(1, &this->id);
        if (colorTexture != 0) {
            glDeleteTextures(1, &colorTexture);
        }
        if (depthTexture != 0) {
            glDeleteTextures(1, &depthTexture);
        }
    }

    void FrameBuffer::bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, this->id);
        glViewport(0, 0, width, height);
    }

    void FrameBuffer::unBind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::resize(uint32_t width, uint32_t height) {
        if (width == 0 && height == 0) {
            return;
        }
        init(id, width, height);
    }

    void FrameBuffer::attachColorTexture() {
        attachColorTexture(width, height);
    }

    void FrameBuffer::attachColorTexture(uint32_t width, uint32_t height) {
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }


    void FrameBuffer::attachDepthTexture() {
        attachDepthTexture(width, height);
    }

    void FrameBuffer::attachDepthTexture(uint32_t width, uint32_t height) {
        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    void FrameBuffer::clear() {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void FrameBuffer::bindShadowMapTexture() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
    }

    uint32_t FrameBuffer::getColorAttachmentRenderer() {
        return colorTexture;
    }

    uint32_t FrameBuffer::getID() {
        return id;
    }
}