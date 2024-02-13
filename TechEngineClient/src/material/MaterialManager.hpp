#pragma once

#include <unordered_map>
#include <string>
#include "Material.hpp"
#include "renderer/TextureManager.hpp"

namespace TechEngine {
    class MaterialManager {
    private:
        TextureManager& m_textureManager;
        std::unordered_map<std::string, Material> m_materialsBank = std::unordered_map<std::string, Material>();

        glm::vec4 m_defaultColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec3 m_defaultAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
        glm::vec3 m_defaultDiffuse = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 m_defaultSpecular = glm::vec3(1.0f, 1.0f, 1.0f);
        float m_defaultShininess = 32.0f;

    public:
        explicit MaterialManager(TextureManager& textureManager);

        void init(const std::vector<std::string>& materialsFilePaths);

        Material& createMaterial(const std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        Material& createMaterial(const std::string& name, Texture* diffuse);

        Material& createMaterialFile(const std::string& name, const std::string& filepath);

        bool deleteMaterial(const std::string& name);

        bool materialExists(const std::string& name);

        Material& getMaterial(const std::string& name);

        std::vector<Material*> getMaterials();

        void serializeMaterial(const std::string& name, const std::string& filepath);

        void clear();

    private:
        bool deserializeMaterial(const std::string& filepath);
    };
}
