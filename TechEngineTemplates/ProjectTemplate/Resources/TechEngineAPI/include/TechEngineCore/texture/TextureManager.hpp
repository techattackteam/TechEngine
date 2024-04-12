#pragma once

#include "Texture.hpp"
#include <unordered_map>

namespace TechEngine {
    class TextureManager {
    private:
        std::unordered_map<std::string, Texture> m_texturesBank = std::unordered_map<std::string, Texture>();
    public:
        void init(const std::vector<std::string> &texturesFilePaths);

        Texture &createTexture(const std::string &name, const std::string &filepath);

        bool deleteTexture(const std::string &name);

        Texture &getTexture(const std::string &name);
    };
}
