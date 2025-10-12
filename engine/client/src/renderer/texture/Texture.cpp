#include "Texture.hpp"

#include <GL/glew.h>

#include "core/Logger.hpp"
#include "TechEngine/core/resources/texture/TextureResource.hpp"

namespace TechEngine {
    Texture::Texture() : m_id(0), m_handle(0), m_isResident(false) {
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
        if ((textureType == PNG || textureType == JPG) && (channels == 3 || channels == 4)) {
            internalFormat = (channels == 3) ? GL_RGB8 : GL_RGBA8;
            format = (channels == 3) ? GL_RGB : GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            pixels = textureResource.getPixelsChar();
        } else if (textureType == HDR && (channels == 3 || channels == 4)) {
            internalFormat = (channels == 3) ? GL_RGB16F : GL_RGBA16F;
            format = (channels == 3) ? GL_RGB : GL_RGBA;
            type = GL_FLOAT;
            pixelsFloat = textureResource.getPixelsFloat();
        } else {
            TE_LOGGER_CRITICAL("Failed to upload texture: Unsupported type or channels: {0}", channels);
            m_id = 0;
            return;
        }

        create(GL_TEXTURE_2D, internalFormat, textureResource.getWidth(), textureResource.getHeight(), format, type,
               (type == GL_FLOAT) ? static_cast<const void*>(pixelsFloat.data()) : static_cast<const void*>(pixels.data()));
        glGenerateTextureMipmap(m_id);
    }

    void Texture::create(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data) {
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
        } else { // GL_TEXTURE_2D
            glTexImage2D(m_target, 0, internalFormat, width, height, 0, format, type, data);
        }

        glTexParameteri(m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        if (m_target == GL_TEXTURE_CUBE_MAP) {
            glTexParameteri(m_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }

        glTexParameteri(m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(m_target, 0);
        m_isResident = false;
        m_handle = 0;
    }

    void Texture::generateMipMap() {
        glGenerateMipmap(m_target);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
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

    bool Texture::isResident() const {
        return m_isResident;
    }
}
