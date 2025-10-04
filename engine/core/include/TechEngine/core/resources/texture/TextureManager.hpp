#pragma once
#define STB_IMAGE_IMPLEMENTATION
#include <filesystem>

#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/resources/texture/TextureResource.hpp"

namespace TechEngine {
    class CORE_DLL TextureManager {
    private:
        std::unordered_map<std::string, TextureResource> m_texturesBank = std::unordered_map<std::string, TextureResource>();
        SystemsRegistry& m_systemsRegistry;

    public:
        TextureManager(SystemsRegistry& systemsRegistry);

        void init(const std::vector<std::filesystem::path>& textureFilePaths);

        void shutdown();

        bool loadFromFile(const std::string& name, const std::filesystem::path& path);

        bool textureExists(const std::string& name);

        TextureResource& getTexture(const std::string& name);

        TextureResource& getTexture(const int id);
    };
}
