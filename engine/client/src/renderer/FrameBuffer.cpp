#include "FrameBuffer.hpp"

#include <glm/gtc/type_ptr.inl>

#include "core/Logger.hpp"
#include "resources/Texture.hpp"

namespace TechEngine {
    FrameBuffer::FrameBuffer(const FrameBuffer& other) {
        // Copy dimensions
        m_width = other.m_width;
        m_height = other.m_height;

        // Generate new OpenGL framebuffer
        glGenFramebuffers(1, &m_id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_id);

        // Deep copy textures
        for (const auto& [attachmentPoint, texPtr]: other.m_textures) {
            auto newTexture = std::make_unique<Texture>(*texPtr);

            // Re-attach the cloned texture
            GLenum target = newTexture->getTarget();
            if (target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_MULTISAMPLE) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, target, newTexture->getID(), 0);
            } else if (target == GL_TEXTURE_CUBE_MAP) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X, newTexture->getID(), 0);
            } else {
                glFramebufferTexture(GL_FRAMEBUFFER, attachmentPoint, newTexture->getID(), 0);
            }

            m_textures[attachmentPoint] = std::move(newTexture);
        }

        // Deep copy renderbuffers
        for (const auto& [attachmentPoint, rbo]: other.m_renderBuffers) {
            const auto& info = other.m_attachments.at(attachmentPoint);
            attachRenderBuffer(attachmentPoint, info.internalFormat, m_width, m_height, info.multisample ? 4 : 0);
        }

        // Copy attachment metadata
        m_attachments = other.m_attachments;

        // External textures are NOT copied (they're references to external objects)
        // m_externalTextures intentionally left empty

        updateDrawBuffers();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    FrameBuffer& FrameBuffer::operator=(const FrameBuffer& other) {
        if (this == &other) return *this;

        // Release current resources
        release();

        // Use copy constructor logic
        m_width = other.m_width;
        m_height = other.m_height;

        glGenFramebuffers(1, &m_id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_id);

        for (const auto& [attachmentPoint, texPtr]: other.m_textures) {
            auto newTexture = std::make_unique<Texture>(*texPtr);
            GLenum target = newTexture->getTarget();
            if (target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_MULTISAMPLE) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, target, newTexture->getID(), 0);
            } else if (target == GL_TEXTURE_CUBE_MAP) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X, newTexture->getID(), 0);
            } else {
                glFramebufferTexture(GL_FRAMEBUFFER, attachmentPoint, newTexture->getID(), 0);
            }
            m_textures[attachmentPoint] = std::move(newTexture);
        }

        for (const auto& [attachmentPoint, rbo]: other.m_renderBuffers) {
            const auto& info = other.m_attachments.at(attachmentPoint);
            attachRenderBuffer(attachmentPoint, info.internalFormat, m_width, m_height, info.multisample ? 4 : 0);
        }

        m_attachments = other.m_attachments;

        updateDrawBuffers();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return *this;
    }


    FrameBuffer::~FrameBuffer() {
        release();
    }

    void FrameBuffer::init(uint32_t width, uint32_t height) {
        this->m_width = width;
        this->m_height = height;
        glGenFramebuffers(1, &this->m_id);
    }

    void FrameBuffer::release() {
        if (m_id) {
            for (auto& [_, texPtr]: m_textures) {
                texPtr->deleteTexture();
            }
            m_textures.clear();

            for (auto& [_, rbo]: m_renderBuffers) {
                if (rbo) {
                    glDeleteRenderbuffers(1, &rbo);
                }
            }
            m_renderBuffers.clear();

            glDeleteFramebuffers(1, &m_id);
            m_id = 0;
        }

        m_attachments.clear();
        m_externalTextures.clear();
        m_width = m_height = 0;
    }


    void FrameBuffer::bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, m_id);
        glViewport(0, 0, m_width, m_height);
    }

    void FrameBuffer::bindDraw() const {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_id);
        glViewport(0, 0, m_width, m_height);
    }

    void FrameBuffer::bindRead() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_id);
    }

    void FrameBuffer::unBind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::resize(uint32_t newWidth, uint32_t newHeight) {
        if (newWidth == 0 || newHeight == 0) return;
        if (newWidth == m_width && newHeight == m_height) return;

        m_width = newWidth;
        m_height = newHeight;

        for (auto& [attachmentPoint, info]: m_attachments) {
            if (info.isRenderBuffer) {
                if (m_renderBuffers.contains(attachmentPoint)) {
                    glDeleteRenderbuffers(1, &m_renderBuffers.at(attachmentPoint));
                    m_renderBuffers.erase(attachmentPoint);
                }
                attachRenderBuffer(attachmentPoint, info.internalFormat, m_width, m_height, info.multisample ? 4 : 0);
            } else {
                if (m_textures.contains(attachmentPoint)) {
                    auto& texPtr = m_textures[attachmentPoint];
                    GLenum target = info.target;
                    texPtr->deleteTexture();
                    if (info.multisample) {
                        attachTexture(attachmentPoint, target, info.internalFormat, info.format, info.type, true, 4);
                    } else {
                        attachTexture(attachmentPoint, target, info.internalFormat, info.format, info.type, false, 0);
                    }
                } else {
                    TE_LOGGER_WARN("External attachment at {0} not recreated on resize; external textures must be updated by caller.", attachmentPoint);
                }
            }
        }
        updateDrawBuffers();
    }


    void FrameBuffer::clear(GLbitfield mask) {
        glClear(mask);
    }

    uint32_t FrameBuffer::attachTexture(GLenum attachmentPoint, GLenum target, GLint internalFormat, GLenum format, GLenum type, bool multisample, int samples) {
        if (m_id == 0) {
            TE_LOGGER_CRITICAL("Attempt to attach texture to uninitialized FrameBuffer");
            return 0;
        }

        auto texture = std::make_unique<Texture>();

        if (multisample) {
            texture->create(target, internalFormat, m_width, m_height, format, type, GL_CLAMP, GL_CLAMP, nullptr); // create will use glTexImage2D normally
            glBindTexture(target, texture->getID());
            glTexImage2DMultisample(target, samples, internalFormat, m_width, m_height, GL_TRUE);
            glBindTexture(target, 0);
        } else {
            texture->create(target, internalFormat, m_width, m_height, format, type, GL_CLAMP, GL_CLAMP, nullptr);
        }

        if (target == GL_TEXTURE_2D || target == GL_TEXTURE_2D_MULTISAMPLE) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, target, texture->getID(), 0);
        } else if (target == GL_TEXTURE_CUBE_MAP) {
            glFramebufferTexture(GL_FRAMEBUFFER, attachmentPoint, texture->getID(), 0);
        } else {
            glFramebufferTexture(GL_FRAMEBUFFER, attachmentPoint, texture->getID(), 0);
        }

        AttachmentInfo info;
        info.isRenderBuffer = false;
        info.internalFormat = internalFormat;
        info.format = format;
        info.type = type;
        info.target = target;
        info.multisample = multisample;

        m_attachments[attachmentPoint] = info;
        m_textures[attachmentPoint] = std::move(texture);

        updateDrawBuffers();

        if (!validate()) {
            TE_LOGGER_ERROR("Framebuffer not complete after attaching texture (attachment {0}).", attachmentPoint);
        }

        return m_textures[attachmentPoint]->getID();
    }

    void FrameBuffer::attachExternalTexture(GLenum attachmentPoint, const Texture& texture, int layer, int mipLevel) {
        if (m_id == 0) {
            TE_LOGGER_CRITICAL("Attempt to attach external texture to uninitialized FrameBuffer");
            return;
        }

        GLenum target = texture.getTarget();
        if (target == GL_TEXTURE_2D) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, target, texture.getID(), mipLevel);
        } else if (target == GL_TEXTURE_CUBE_MAP) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, texture.getID(), mipLevel);
        } else {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, attachmentPoint, texture.getID(), mipLevel, layer);
        }

        AttachmentInfo info;
        info.isRenderBuffer = false;
        info.internalFormat = 0; // unknown (external)
        info.format = 0;
        info.type = 0;
        info.target = target;
        info.multisample = false;

        m_attachments[attachmentPoint] = info;
        m_externalTextures[attachmentPoint] = &texture;

        updateDrawBuffers();

        if (!validate()) {
            TE_LOGGER_ERROR("Framebuffer not complete after attaching external texture (attachment {0}).", attachmentPoint);
        }
    }

    void FrameBuffer::attachTextureLayer(GLenum attachmentPoint, const Texture& texture, int layer, int mipLevel) {
        GLenum target = texture.getTarget();
        if (target == GL_TEXTURE_CUBE_MAP) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, texture.getID(), mipLevel);
        } else {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, attachmentPoint, texture.getID(), mipLevel, layer);
        }

        AttachmentInfo info;
        info.isRenderBuffer = false;
        info.internalFormat = 0;
        info.target = target;
        m_attachments[attachmentPoint] = info;
        m_externalTextures[attachmentPoint] = &texture;

        updateDrawBuffers();
        if (!validate()) {
            TE_LOGGER_ERROR("Framebuffer not complete after attachTextureLayer (attachment {0}).", attachmentPoint);
        }
    }

    uint32_t FrameBuffer::attachRenderBuffer(GLenum attachmentPoint, GLint internalFormat, uint32_t width, uint32_t height, int samples) {
        if (m_id == 0) {
            TE_LOGGER_CRITICAL("Attempt to attach renderbuffer to uninitialized FrameBuffer");
            return 0;
        }

        if (m_renderBuffers.contains(attachmentPoint)) {
            glDeleteRenderbuffers(1, &m_renderBuffers.at(attachmentPoint));
            m_renderBuffers.erase(attachmentPoint);
        }

        uint32_t rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        if (samples > 0) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internalFormat, width, height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height);
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        AttachmentInfo info;
        info.isRenderBuffer = true;
        info.internalFormat = internalFormat;
        info.multisample = (samples > 0);
        m_attachments[attachmentPoint] = info;
        m_renderBuffers[attachmentPoint] = rbo;

        updateDrawBuffers();
        if (!validate()) {
            TE_LOGGER_ERROR("Framebuffer not complete after attaching renderbuffer (attachment {0}).", attachmentPoint);
        }
        return rbo;
    }

    bool FrameBuffer::validate() const {
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status == GL_FRAMEBUFFER_COMPLETE) return true;

        switch (status) {
            case GL_FRAMEBUFFER_UNDEFINED: TE_LOGGER_ERROR("Framebuffer incomplete: GL_FRAMEBUFFER_UNDEFINED");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: TE_LOGGER_ERROR("Framebuffer incomplete: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: TE_LOGGER_ERROR("Framebuffer incomplete: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: TE_LOGGER_ERROR("Framebuffer incomplete: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: TE_LOGGER_ERROR("Framebuffer incomplete: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED: TE_LOGGER_ERROR("Framebuffer incomplete: GL_FRAMEBUFFER_UNSUPPORTED");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: TE_LOGGER_ERROR("Framebuffer incomplete: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: TE_LOGGER_ERROR("Framebuffer incomplete: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
                break;
            default: TE_LOGGER_ERROR("Framebuffer incomplete: Unknown status {0}", status);
                break;
        }
        return false;
    }

    uint32_t FrameBuffer::getID() {
        return m_id;
    }

    uint32_t FrameBuffer::getWidth() const {
        return m_width;
    }

    uint32_t FrameBuffer::getHeight() const {
        return m_height;
    }

    glm::uvec2 FrameBuffer::getSize() const {
        return glm::uvec2(m_width, m_height);
    }

    Texture* FrameBuffer::getTexture(GLenum attachmentPoint) {
        if (m_textures.contains(attachmentPoint)) {
            //This can create a problem with dangling pointers if the texture is reattached or the framebuffer is resized.
            return m_textures[attachmentPoint].get();
        }
        if (m_externalTextures.contains(attachmentPoint)) {
            return const_cast<Texture*>(m_externalTextures[attachmentPoint]);
        }
        return nullptr;
    }

    const Texture* FrameBuffer::getTexture(GLenum attachmentPoint) const {
        if (m_textures.contains(attachmentPoint)) {
            //This can create a problem with dangling pointers if the texture is reattached or the framebuffer is resized.
            return m_textures.at(attachmentPoint).get();
        }
        if (m_externalTextures.contains(attachmentPoint)) {
            return m_externalTextures.at(attachmentPoint);
        }
        return nullptr;
    }

    uint32_t FrameBuffer::getTextureID(GLenum attachmentPoint) const {
        const Texture* tex = getTexture(attachmentPoint);
        return tex ? tex->getID() : 0;
    }

    bool FrameBuffer::hasTexture(GLenum attachmentPoint) const {
        return m_textures.contains(attachmentPoint) || m_externalTextures.contains(attachmentPoint);
    }

    bool FrameBuffer::hasAttachment(GLenum attachmentPoint) const {
        return m_attachments.contains(attachmentPoint);
    }

    void FrameBuffer::updateDrawBuffers() {
        std::vector<GLenum> drawBuffers;
        for (const auto& [att, info]: m_attachments) {
            if (att >= GL_COLOR_ATTACHMENT0 && att <= GL_COLOR_ATTACHMENT15) {
                drawBuffers.push_back(att);
            }
        }

        if (drawBuffers.empty()) {
            GLenum none = GL_NONE;
            glDrawBuffers(1, &none);
        } else {
            glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
        }
    }
}
