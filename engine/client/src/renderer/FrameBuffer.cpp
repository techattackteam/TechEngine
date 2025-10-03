#include "FrameBuffer.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    void FrameBuffer::init(uint32_t id, uint32_t width, uint32_t height) {
        this->width = width;
        this->height = height;
        if (this->id != 0) {
            glDeleteFramebuffers(1, &id);
        }
        this->id = id;
        glGenFramebuffers(1, &this->id);
        glViewport(0, 0, width, height);
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
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Bind to default framebuffer
    }

    void FrameBuffer::resize(uint32_t newWidth, uint32_t newHeight) {
        if (newWidth == 0 || newHeight == 0) {
            return;
        }

        // Store which attachments we had
        bool hadColorTexture = (colorTexture != 0);
        bool hadDepthTexture = (depthTexture != 0);
        bool hadDepthCubeMap = (depthCubeMapTexture != 0);

        // Delete existing textures
        if (colorTexture != 0) {
            glDeleteTextures(1, &colorTexture);
            colorTexture = 0;
        }
        if (depthTexture != 0) {
            glDeleteTextures(1, &depthTexture);
            depthTexture = 0;
        }
        if (depthCubeMapTexture != 0) {
            glDeleteTextures(1, &depthCubeMapTexture);
            depthCubeMapTexture = 0;
        }

        width = newWidth;
        height = newHeight;

        glBindFramebuffer(GL_FRAMEBUFFER, id);

        if (hadColorTexture) {
            attachColorTexture(width, height);
        }
        if (hadDepthTexture) {
            attachDepthTexture(width, height);
        }
        if (hadDepthCubeMap) {
            attachDepthCubeMapTexture(width, height);
        }

        glViewport(0, 0, width, height);
    }

    void FrameBuffer::attachColorTexture() {
        attachColorTexture(width, height);
    }

    void FrameBuffer::attachColorTexture(uint32_t width, uint32_t height) {
        glGenTextures(1, &colorTexture);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }

    void FrameBuffer::attachDepthCubeMapTexture(uint32_t width, uint32_t height) {
        glGenTextures(1, &depthCubeMapTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMapTexture);
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMapTexture, 0);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            // Log error with status code
            TE_LOGGER_CRITICAL("Framebuffer not complete after attaching depth cube map! Status: 0x%x", status);
            throw std::runtime_error("Framebuffer not complete after attaching depth cube map!");
        }
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
