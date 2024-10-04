#pragma once

#include <filesystem>

#include "Material.hpp"
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace TechEngine {
    class CORE_DLL MaterialManager {
    public:
        inline static const std::string DEFAULT_MATERIAL_NAME = "Default";

    private:
        std::unordered_map<std::string, Material> m_materialsBank = std::unordered_map<std::string, Material>();

        glm::vec4 m_defaultColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec3 m_defaultAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
        glm::vec3 m_defaultDiffuse = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 m_defaultSpecular = glm::vec3(1.0f, 1.0f, 1.0f);
        float m_defaultShininess = 32.0f;

        MaterialManager* m_copy = nullptr;

    public:
        explicit MaterialManager();

        void init(const std::vector<std::filesystem::path>& materialsFilePaths);

        void shutdown();

        Material& createMaterial(const ::std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        Material& createMaterial(const std::string& name);

        /*Material& createMaterial(const std::string& name, Texture* diffuse);*/

        Material& createMaterialFile(const std::string& name, const std::string& filepath);

        bool deleteMaterial(const std::string& name);

        bool materialExists(const std::string& name);

        Material& getMaterial(const std::string& name);

        std::vector<Material*> getMaterials();

    private:
        bool registerMaterial(const std::string& filepath);

        void serializeMaterial(const std::string& name, const std::string& filepath);
    };
}
