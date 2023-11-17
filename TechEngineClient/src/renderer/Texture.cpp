#include "Texture.hpp"
#include "GL/glew.h"
#include "core/Logger.hpp"
#include "core/FileSystem.hpp"
#include "ErrorCatcher.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

namespace TechEngine {

    Texture::Texture(const std::string &filepath) {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc *data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

        if (data) {

            GLenum internalFormat = 0, dataFormat = 0;
            if (channels == 4) {
                internalFormat = GL_RGBA8;
                dataFormat = GL_RGBA;
            } else if (channels == 3) {
                internalFormat = GL_RGB;
                dataFormat = GL_RGB;
            } else if (channels == 2) {
                internalFormat = GL_RG;
                dataFormat = GL_RG;
            } else if (channels == 1) {
                internalFormat = GL_RED;
                dataFormat = GL_RED;
            }


            GlCall(glGenTextures(1, &m_id));
            GlCall(glBindTexture(GL_TEXTURE_2D, m_id));
            //GlCall(glTextureStorage2D(m_id, 1, internalFormat, m_width, m_height));

            GlCall(glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GlCall(glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GlCall(glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, GL_REPEAT));
            GlCall(glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, GL_REPEAT));

            //glTextureSubImage2D(m_id, 0, 0, 0, m_width, m_height, dataFormat, GL_UNSIGNED_BYTE, data);
            GlCall(glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data););
            //GlCall(glGenerateMipmap(GL_TEXTURE_2D));
            stbi_image_free(data);
            TE_LOGGER_INFO("Texture {0} loaded", filepath);
        } else {
            TE_LOGGER_ERROR("Failed to load texture {0}", filepath);
        }
    }

    Texture::~Texture() {
        glDeleteTextures(1, &m_id);
    }

    void Texture::bind(unsigned int slot) const {
        GlCall(glActiveTexture(GL_TEXTURE1));
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

    void Texture::unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

}