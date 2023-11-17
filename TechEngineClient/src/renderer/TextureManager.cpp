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
        Texture *texture = new Texture(filepath);
        auto iterator = getInstance().m_texturesBank.find(name);
        if (iterator == getInstance().m_texturesBank.end()) {
            iterator = getInstance().m_texturesBank.emplace(name, texture).first;
        }
        return *iterator->second;
    }

    bool TextureManager::deleteTexture(const std::string &name) {
        if (getInstance().m_texturesBank.find(name) != getInstance().m_texturesBank.end()) {
            getInstance().m_texturesBank.erase(name);
            return true;
        }
        return false;
    }

    Texture &TextureManager::getTexture(const std::string &name) {
        if (getInstance().m_texturesBank.find(name) == getInstance().m_texturesBank.end())
            TE_LOGGER_ERROR("Texture {0} not found", name);
        return *getInstance().m_texturesBank.at(name);
    }
}