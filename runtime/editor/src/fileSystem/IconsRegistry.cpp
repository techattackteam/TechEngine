#include "IconsRegistry.hpp"

#include <stb_image.h>
#include <GL/glew.h>

#include "core/Logger.hpp"
#include "TechEngine/core/resources/texture/TextureResource.hpp"

namespace TechEngine {
    IconRegistry::IconRegistry(FileSystem& fileSystem) : m_fileSystem(fileSystem) {
    }

    IconRegistry::~IconRegistry() {
        releaseAll();
    }

    IconEntry IconRegistry::getIcon(const std::filesystem::path& virtualIconPath) {
        auto it = m_iconCache.find(virtualIconPath);
        if (it != m_iconCache.end()) {
            return it->second;
        }

        IconEntry entry = loadFromDisk(virtualIconPath);
        m_iconCache[virtualIconPath] = entry;
        return entry;
    }

    bool IconRegistry::buildThumbnailFromPixelData(UUID resourceUUID, const TextureResource& texture) {
        if (!texture.hasPixelData()) {
            TE_LOGGER_WARN("[IconRegistry] Cannot build thumbnail for '{0}': pixel data has been freed.", texture.getName());
            return false;
        }

        // Determine GL format from channel count.
        // TextureResource only stores 3 or 4 channel data in practice, but handle 1/2 defensively.
        const int channels = texture.getChannels();
        GLenum format;
        switch (channels) {
            case 1: format = GL_RED;
                break;
            case 2: format = GL_RG;
                break;
            case 3: format = GL_RGB;
                break;
            case 4: format = GL_RGBA;
                break;
            default:
                TE_LOGGER_WARN("[IconRegistry] Unsupported channel count ({0}) for thumbnail '{1}'.", channels, texture.getName());
                return false;
        }

        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        if (texture.getType() == TextureType::HDR) {
            const std::vector<float>& pixels = texture.getPixelsFloat();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F,
                         texture.getWidth(), texture.getHeight(),
                         0, format, GL_FLOAT, pixels.data());
        } else {
            const std::vector<unsigned char>& pixels = texture.getPixelsChar();
            glTexImage2D(GL_TEXTURE_2D, 0, format,
                         texture.getWidth(), texture.getHeight(),
                         0, format, GL_UNSIGNED_BYTE, pixels.data());
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        ImTextureID id = reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(texId));
        setThumbnail(resourceUUID, IconEntry{id, glm::vec2{texture.getWidth(), texture.getHeight()}});
        return true;
    }


    void IconRegistry::setThumbnail(UUID resourceUUID, IconEntry texture) {
        auto it = m_thumbnailCache.find(resourceUUID);
        if (it != m_thumbnailCache.end()) {
            deleteGLTexture(it->second.id);
        }

        m_thumbnailCache[resourceUUID] = texture;
    }

    IconEntry IconRegistry::getThumbnail(UUID resourceUUID, const std::filesystem::path& fallbackIconPath) {
        auto it = m_thumbnailCache.find(resourceUUID);
        if (it != m_thumbnailCache.end()) {
            return it->second;
        }

        return getIcon(fallbackIconPath);
    }

    bool IconRegistry::hasThumbnail(UUID resourceUUID) const {
        return m_thumbnailCache.find(resourceUUID) != m_thumbnailCache.end();
    }

    void IconRegistry::releaseThumbnail(UUID resourceUUID) {
        auto it = m_thumbnailCache.find(resourceUUID);
        if (it != m_thumbnailCache.end()) {
            deleteGLTexture(it->second.id);
            m_thumbnailCache.erase(it);
        }
    }

    void IconRegistry::releaseAll() {
        for (auto& [path, entry]: m_iconCache) {
            deleteGLTexture(entry.id);
        }
        for (auto& [uuid, entry]: m_thumbnailCache) {
            deleteGLTexture(entry.id);
        }
        m_iconCache.clear();
        m_thumbnailCache.clear();
    }

    IconEntry IconRegistry::loadFromDisk(const std::filesystem::path& virtualIconPath) const {
        std::filesystem::path absoluteImagePath = m_fileSystem.resolve(virtualIconPath.string());
        if (absoluteImagePath.empty()) {
            TE_LOGGER_WARN("[IconRegistry] Icon not found: {0}", virtualIconPath .string());
            return {};
        }
        int width;
        int height;
        int channels;
        stbi_uc* data = stbi_load(absoluteImagePath.string().c_str(), &width, &height, &channels, 4);

        if (!data) {
            TE_LOGGER_WARN("[IconRegistry] stb failed to decode icon: {0}", virtualIconPath.string());
            return {};
        }

        GLenum format = GL_RGBA;

        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);

        return IconEntry{reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(texId)), glm::vec2(width, height)};
    }

    void IconRegistry::deleteGLTexture(ImTextureID id) {
        if (!id) {
            return;
        }
        GLuint texId = static_cast<GLuint>(reinterpret_cast<uintptr_t>(id));
        glDeleteTextures(1, &texId);
    }
}
