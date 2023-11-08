#pragma once

#include <unordered_map>
#include <string>
#include "Material.hpp"

namespace TechEngine {
    class MaterialManager {
    private:
        std::unordered_map<std::string, Material> m_materialsBank = std::unordered_map<std::string, Material>();

        glm::vec4 m_defaultColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec3 m_defaultAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
        glm::vec3 m_defaultDiffuse = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 m_defaultSpecular = glm::vec3(1.0f, 1.0f, 1.0f);
        float m_defaultShininess = 32.0f;

    public:


        static void init(const std::vector<std::string> &materialsFilePaths);

        static Material &createMaterial(const std::string &name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        static Material &createMaterialFile(const std::string &name, const std::string &filepath);

        static bool deleteMaterial(const std::string &name);

        static Material &getMaterial(const std::string &name);

        static void serializeMaterial(const std::string &name, const std::string &filepath);

    private:
        MaterialManager() = default;


        static bool deserializeMaterial(const std::string &filepath);

        MaterialManager(const MaterialManager &) = delete;

        MaterialManager &operator=(const MaterialManager &) = delete;

        static MaterialManager &getInstance() {
            static MaterialManager instance;
            return instance;
        }
    };
}
