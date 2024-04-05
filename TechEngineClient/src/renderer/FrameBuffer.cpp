#include "FrameBuffer.hpp"
#include "ErrorCatcher.hpp"
#include "RendererSettings.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/window/FramebufferResizeEvent.hpp"

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
        GlCall(glGenFramebuffers(1, &this->id));
        glViewport(0, 0, width, height);
        bind();
        //Todo: Transfer this to the renderer init without for some reason crashing
        attachDepthTexture();
        attachColorTexture();
    }


    FrameBuffer::~FrameBuffer() {
        GlCall(glDeleteFramebuffers(1, &this->id));
        if (colorTexture != 0) {
            GlCall(glDeleteTextures(1, &colorTexture));
        }
        if (depthTexture != 0) {
            GlCall(glDeleteTextures(1, &depthTexture));
        }
    }

    void FrameBuffer::bind() {
        GlCall(glBindFramebuffer(GL_FRAMEBUFFER, this->id));
        RendererSettings::targetId = id;
        glViewport(0, 0, width, height);
    }

    void FrameBuffer::unBind() {
        GlCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        RendererSettings::targetId = 0;
    }

    void FrameBuffer::resize(uint32_t width, uint32_t height) {
        if (width == 0 && height == 0) {
            return;
        }
        if (RendererSettings::targetId == id) {
            RendererSettings::resize(width, height);
            EventDispatcher::getInstance().dispatch(new FramebufferResizeEvent(id, width, height));
        }
        init(id, width, height);
    }

    void FrameBuffer::attachColorTexture() {
        attachColorTexture(width, height);
    }

    void FrameBuffer::attachColorTexture(uint32_t width, uint32_t height) {
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
    }


    void FrameBuffer::attachDepthTexture() {
        attachDepthTexture(width, height);
    }

    void FrameBuffer::attachDepthTexture(uint32_t width, uint32_t height) {
        GlCall(glGenTextures(1, &depthTexture));
        GlCall(glBindTexture(GL_TEXTURE_2D, depthTexture));
        GlCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));

        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        GlCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

        GlCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0));
        GlCall(glDrawBuffer(GL_NONE));
        GlCall(glReadBuffer(GL_NONE));
    }

    void FrameBuffer::clear() {
        GlCall(glClear(GL_DEPTH_BUFFER_BIT));
    }

    void FrameBuffer::bindShadowMapTexture() {
        GlCall(glActiveTexture(GL_TEXTURE0));
        GlCall(glBindTexture(GL_TEXTURE_2D, depthTexture));
    }

    uint32_t FrameBuffer::getColorAttachmentRenderer() {
        return colorTexture;
    }

    uint32_t FrameBuffer::getID() {
        return id;
    }
}