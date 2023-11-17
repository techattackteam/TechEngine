#pragma once

#include "Texture.hpp"
#include <unordered_map>

namespace TechEngine {
    class TextureManager {
    private:
        std::unordered_map<std::string, Texture> m_texturesBank = std::unordered_map<std::string, Texture>();
    public:
        static void init(const std::vector<std::string> &texturesFilePaths);

        static Texture &createTexture(const std::string &name, const std::string &filepath);

        static bool deleteTexture(const std::string &name);

        static Texture &getTexture(const std::string &name);

    private:
        TextureManager() = default;

        TextureManager(const TextureManager &) = delete;

        TextureManager &operator=(const TextureManager &) = delete;

        static TextureManager &getInstance() {
            static TextureManager instance;
            return instance;
        }
    };
}
