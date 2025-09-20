#pragma once

#include "Material.hpp"

#include <filesystem>
#include <unordered_map>

#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    class CORE_DLL MaterialManager {
    public:
        inline static const std::string DEFAULT_MATERIAL_NAME = "Default";

    private:
        glm::vec4 m_defaultColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        glm::vec3 m_defaultAmbient = glm::vec3(0.2f, 0.2f, 0.2f);
        glm::vec3 m_defaultDiffuse = glm::vec3(0.5f, 0.5f, 0.5f);
        glm::vec3 m_defaultSpecular = glm::vec3(1.0f, 1.0f, 1.0f);
        float m_defaultShininess = 32.0f;

        SystemsRegistry& m_systemsRegistry;
        MaterialManager* m_copy = nullptr;


        std::unordered_map<std::string, Material> m_materialsBank = std::unordered_map<std::string, Material>();
        uint32_t gpuID = 0;
        std::vector<uint32_t> freeIDs;

    public:
        explicit MaterialManager(SystemsRegistry& systemsRegistry);

        void init(const std::vector<std::filesystem::path>& materialsFilePaths);

        void shutdown();

        Material& createMaterial(const std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        Material& createMaterial(const std::string& name);

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
