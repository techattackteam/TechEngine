#include "TechEngine/core/resources/texture/TextureManager.hpp"
#include "stb_image.h"

namespace TechEngine {
    TextureManager::TextureManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void TextureManager::init(const std::vector<std::filesystem::path>& textureFilePaths) {
        for (const auto& path: textureFilePaths) {
            std::string name = path.stem().string();
            if (textureExists(name)) {
                TE_LOGGER_WARN("Texture already registered: {0}", name);
                continue;
            }
            loadFromFile(name, path);
        }
    }

    void TextureManager::shutdown() {
    }

    bool TextureManager::loadFromFile(const std::string& name, const std::filesystem::path& path) {
        m_texturesBank.reserve(10);

        int width, height, channels;
        std::vector<unsigned char> pixelData;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
        if (!data) {
            TE_LOGGER_ERROR("Failed to load texture: {0}", path.string());
            return false;
        }

        pixelData.assign(data, data + (width * height * channels));

        m_texturesBank.emplace(name, TextureResource(name, m_texturesBank.size(), width, height, channels, pixelData));
        stbi_image_free(data);
        return true;
    }

    bool TextureManager::textureExists(const std::string& name) {
        return m_texturesBank.find(name) != m_texturesBank.end();
    }

    TextureResource& TextureManager::getTexture(const std::string& name) {
        return m_texturesBank.at(name);
    }

    TextureResource& TextureManager::getTexture(const int id) {
        for (const auto& [name, texture]: m_texturesBank) {
            if (texture.getID() == id) {
                return m_texturesBank.at(name);
            }
        }
        throw std::runtime_error("Texture not found");
    }
}
