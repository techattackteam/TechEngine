#include "Texture.hpp"
#include "core/Logger.hpp"


namespace TechEngine {
    Texture::Texture() : m_id(0), m_handle(0), m_isResident(false), IGPUResource(-1) {
    }

    Texture::Texture(const UUID& resourceUUID) : m_id(0), m_handle(0), m_isResident(false), IGPUResource(resourceUUID) {
    }

    void Texture::uploadFromResource(const TextureResource& textureResource) {
        if (textureResource.getWidth() == 0 || textureResource.getHeight() == 0) {
            TE_LOGGER_CRITICAL("Failed to upload texture: Invalid texture resource");
            return;
        }
        int channels = textureResource.getChannels();
        TextureType textureType = textureResource.getType();

        GLint internalFormat;
        GLenum format;
        GLenum type;
        std::vector<unsigned char> pixels;
        std::vector<float> pixelsFloat;
        if (textureType == PNG || textureType == JPG) {
            if (channels == 1) {
                internalFormat = GL_R8;
                format = GL_RED;
            } else if (channels == 2) {
                internalFormat = GL_RG8;
                format = GL_RG;
            } else if (channels == 3) {
                internalFormat = GL_SRGB8;
                format = GL_RGB;
            } else if (channels == 4) {
                internalFormat = GL_RGBA8;
                format = GL_RGBA;
            }
            type = GL_UNSIGNED_BYTE;
            pixels = textureResource.getPixelsChar();
        } else if (textureType == HDR) {
            if (channels == 1) {
                internalFormat = GL_R8;
                format = GL_RED;
            } else if (channels == 2) {
                internalFormat = GL_RG8;
                format = GL_RG;
            } else if (channels == 3) {
                internalFormat = GL_RGB8;
                format = GL_RGB;
            } else if (channels == 4) {
                internalFormat = GL_RGBA8;
                format = GL_RGBA;
            }
            type = GL_FLOAT;
            pixelsFloat = textureResource.getPixelsFloat();
        } else {
            TE_LOGGER_CRITICAL("Failed to upload texture: Unsupported type or channels: {0}", channels);
            m_id = 0;
            return;
        }

        create(GL_TEXTURE_2D, internalFormat, textureResource.getWidth(), textureResource.getHeight(), format, type,
               getOpenGlWrapMode(textureResource.getModeU()), getOpenGlWrapMode(textureResource.getModeV()),
               (type == GL_FLOAT) ? static_cast<const void*>(pixelsFloat.data()) : static_cast<const void*>(pixels.data()));
        glGenerateTextureMipmap(m_id);
    }

    void Texture::create(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLenum wrapU, GLenum wrapV, const void* data, GLsizei depth) {
        if (m_id != 0) {
            deleteTexture();
        }

        m_target = target;
        glGenTextures(1, &m_id);
        glBindTexture(m_target, m_id);

        if (m_target == GL_TEXTURE_CUBE_MAP) {
            for (unsigned int i = 0; i < 6; ++i) {
                // Note: data is expected to be null for initial empty cubemap creation
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, type, data);
            }
        } else if (m_target == GL_TEXTURE_3D) {
            glTexImage3D(m_target, 0, internalFormat, width, height, depth, 0, format, type, data);
        } else { // GL_TEXTURE_2D
            glTexImage2D(m_target, 0, internalFormat, width, height, 0, format, type, data);
        }

        glTexParameteri(m_target, GL_TEXTURE_WRAP_S, wrapU);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_T, wrapV);
        if (m_target == GL_TEXTURE_CUBE_MAP || m_target == GL_TEXTURE_3D) {
            glTexParameteri(m_target, GL_TEXTURE_WRAP_R, wrapV);
        }

        glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(m_target, 0);
        m_isResident = false;
        m_handle = 0;
        m_width = width;
        m_height = height;
    }

    void Texture::generateMipMap() {
        glGenerateMipmap(m_target);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void Texture::makeResident() {
        if (!m_isResident) {
            glBindTexture(GL_TEXTURE_2D, m_id);
            m_handle = glGetTextureHandleARB(m_id);
            glMakeTextureHandleResidentARB(m_handle);
            m_isResident = true;
        }
    }

    void Texture::makeNonResident() {
        if (m_isResident) {
            glMakeTextureHandleNonResidentARB(m_handle);
            m_isResident = false;
            m_handle = 0;
        }
    }

    void Texture::deleteTexture() {
        glBindTexture(GL_TEXTURE_2D, 0);
        if (m_id != 0) {
            if (m_isResident) {
                makeNonResident();
            }
        }
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    uint64_t Texture::getHandle() const {
        return m_handle;
    }

    uint32_t Texture::getID() const {
        return m_id;
    }

    uint32_t Texture::getID() {
        return m_id;
    }

    GLenum Texture::getTarget() const {
        return m_target;
    }

    uint32_t Texture::getWidth() const {
        return m_width;
    }

    uint32_t Texture::getHeight() const {
        return m_height;
    }

    bool Texture::isResident() const {
        return m_isResident;
    }

    GLenum Texture::getOpenGlWrapMode(const TextureMode& mode) {
        switch (mode) {
            case REPEAT:
                return GL_REPEAT;
            case MIRRORED_REPEAT:
                return GL_MIRRORED_REPEAT;
            case CLAMP_TO_EDGE:
                return GL_CLAMP_TO_EDGE;
            case CLAMP_TO_BORDER:
                return GL_CLAMP_TO_BORDER;
            default:
                return GL_REPEAT;
        }
    }
}
