#pragma once

#include "Material.hpp"
#include "systems/System.hpp"
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace TechEngine {
    class CORE_DLL MaterialManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        std::unordered_map<std::string, Material> m_materialsBank = std::unordered_map<std::string, Material>();

        glm::vec4 m_defaultColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec3 m_defaultAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
        glm::vec3 m_defaultDiffuse = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 m_defaultSpecular = glm::vec3(1.0f, 1.0f, 1.0f);
        float m_defaultShininess = 32.0f;

        MaterialManager* m_copy = nullptr;

    public:
        explicit MaterialManager(SystemsRegistry& systemsRegistry);

        void init(const std::vector<std::string>& materialsFilePaths);

        void shutdown() override;

        Material& createMaterial(const ::std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        /*Material& createMaterial(const std::string& name, Texture* diffuse);*/

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
