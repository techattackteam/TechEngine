#include "TextureManager.hpp"
#include "core/Logger.hpp"
#include "core/FileSystem.hpp"

namespace TechEngine {

    void TextureManager::init(const std::vector<std::string> &texturesFilePaths) {
        for (std::string textureFilePath: texturesFilePaths) {
            std::string textureName = FileSystem::getFileName(textureFilePath);
            createTexture(textureName, textureFilePath);
        }
    }

    Texture &TextureManager::createTexture(const std::string &name, const std::string &filepath) {
        Texture texture = Texture(filepath);
        auto result = m_texturesBank.emplace(name, texture);
        return result.first->second;
    }

    bool TextureManager::deleteTexture(const std::string &name) {
        if (m_texturesBank.find(name) != m_texturesBank.end()) {
            m_texturesBank.erase(name);
            return true;
        }
        return false;
    }

    Texture &TextureManager::getTexture(const std::string &name) {
        if (m_texturesBank.find(name) == m_texturesBank.end())
            TE_LOGGER_ERROR("Texture {0} not found", name);
        return m_texturesBank.at(name);
    }
}