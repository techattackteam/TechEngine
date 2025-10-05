#include "Texture.hpp"

#include <GL/glew.h>

#include "core/Logger.hpp"
#include "TechEngine/core/resources/texture/TextureResource.hpp"

namespace TechEngine {
    Texture::Texture() : m_id(0), m_handle(0), m_isResident(false) {
    }

    void Texture::uploadFromResource(const TextureResource& textureResource) {
        const std::vector<unsigned char> pixels = textureResource.getPixels();
        if (!pixels.data() || textureResource.getWidth() == 0 || textureResource.getHeight() == 0) {
            TE_LOGGER_CRITICAL("Failed to upload texture: Invalid texture resource");
            return;
        }
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);
        if (textureResource.getChannels() == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureResource.getWidth(), textureResource.getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, textureResource.getPixels().data());
        } else if (textureResource.getChannels() == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureResource.getWidth(), textureResource.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, textureResource.getPixels().data());
        } else {
            TE_LOGGER_CRITICAL("Failed to upload texture: Unsupported number of channels: {0}", textureResource.getChannels());
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &m_id);
            m_id = 0;
            return;
        }
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_isResident = false;
        m_handle = 0;
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

    uint64_t Texture::getHandle() const {
        return m_handle;
    }

    uint32_t Texture::getID() const {
        return m_id;
    }

    bool Texture::isResident() const {
        return m_isResident;
    }
}
