#include "TechEngine/core/resources/texture/TextureManager.hpp"

#include <ranges>

#include "stb_image.h"

namespace TechEngine {
    class Texture;

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
        stbi_set_flip_vertically_on_load(true);

        std::string extension = path.extension().string();
        TextureType type;

        auto loadTexture = [&](auto* data, auto& pixelContainer, TextureType textureType) {
            if (!data) {
                TE_LOGGER_ERROR("Failed to load texture: {0}", path.string());
                return false;
            }
            pixelContainer.assign(data, data + (width * height * channels));
            m_texturesBank.emplace(name, TextureResource(name, m_texturesBank.size(), width, height, channels, textureType, pixelContainer));
            stbi_image_free(data);
            return true;
        };

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
            std::vector<unsigned char> pixelData;
            type = (extension == ".png") ? PNG : JPG;
            stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
            return loadTexture(data, pixelData, type);
        } else if (extension == ".hdr") {
            std::vector<float> pixelDataFloat;
            type = HDR;
            float* data = stbi_loadf(path.string().c_str(), &width, &height, &channels, 0);
            return loadTexture(data, pixelDataFloat, type);
        } else {
            TE_LOGGER_ERROR("Unsupported texture format: {0}", extension);
            return false;
        }
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

    std::vector<TextureResource*> TextureManager::getTextures() {
        std::vector<TextureResource*> textures;
        for (auto& texture: m_texturesBank | std::views::values) {
            textures.push_back(&texture);
        }
        return textures;
    }

    std::vector<TextureResource*> TextureManager::getTexturesOfType(TextureType type) {
        std::vector<TextureResource*> textures;
        for (auto& texture: m_texturesBank | std::views::values) {
            if (texture.getType() == type) {
                textures.push_back(&texture);
            }
        }
        return textures;
    }
}
