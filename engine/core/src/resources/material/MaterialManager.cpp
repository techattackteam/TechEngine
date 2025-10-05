#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/resources/material/MaterialManager.hpp"
#include "events/resourcersManager/materials/MaterialCreatedEvent.hpp"
#include "events/resourcersManager/materials/MaterialDeletedEvent.hpp"
#include "eventSystem/EventManager.hpp"
#include "resources/mesh/AssimpLoader.hpp"

#include <utils/YAMLUtils.hpp>
#include <fstream>

namespace TechEngine {
    MaterialManager::MaterialManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void MaterialManager::init(const std::vector<std::filesystem::path>& materialsFilePaths) {
        for (const std::filesystem::path& materialFilePath: materialsFilePaths) {
            registerMaterial(materialFilePath.string());
        }
    }

    void MaterialManager::shutdown() {
        m_materialsBank.clear();
    }

    Material& MaterialManager::createMaterial(const std::string& name, glm::vec4 color) {
        if (freeIDs.size() == 0) {
            gpuID = m_materialsBank.size();
        } else {
            gpuID = freeIDs.back();
            freeIDs.pop_back();
        }
        Material material(name, gpuID++, color);
        auto iterator = m_materialsBank.find(name);
        if (iterator == m_materialsBank.end()) {
            iterator = m_materialsBank.emplace(name, material).first;
        }
        m_systemsRegistry.getSystem<EventManager>().dispatch<MaterialCreatedEvent>(name);
        return iterator->second;
    }

    Material& MaterialManager::createMaterial(const std::string& name) {
        return createMaterial(name, m_defaultColor);
    }

    Material& MaterialManager::createMaterialFile(const std::string& name, const std::string& folderPath) {
        std::string filepath = folderPath + "/" + name + ".mat";
        Material& material = createMaterial(name, m_defaultColor);
        serializeMaterial(name, filepath);
        return material;
    }

    bool MaterialManager::deleteMaterial(const std::string& name) {
        if (!m_materialsBank.empty() && materialExists(name)) {
            Material& material = getMaterial(name);
            freeIDs.push_back(material.getGpuID());
            m_materialsBank.erase(name);
            m_systemsRegistry.getSystem<EventManager>().dispatch<MaterialDeletedEvent>(name);
            return true;
        } else {
            TE_LOGGER_WARN("Material {0} does not exist!", name);
        }
        return false;
    }

    bool MaterialManager::materialExists(const std::string& name) {
        return m_materialsBank.contains(name);
    }

    Material& MaterialManager::getMaterial(const std::string& name) {
        if (!materialExists(name)) {
            TE_LOGGER_ERROR("Material {0} not found", name);
        }
        return m_materialsBank.at(name);
    }

    std::vector<Material*> MaterialManager::getMaterials() {
        std::vector<Material*> materials;
        for (auto& [name, material]: m_materialsBank) {
            materials.push_back(&material);
        }
        return materials;
    }

    void MaterialManager::serializeMaterial(const std::string& name, const std::string& filepath) {
        Material& material = getMaterial(name);
        MaterialProperties properties = material.getProperties();
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << name;
        out << YAML::Key << "albedo" << YAML::Value << YAML::Flow << YAML::BeginSeq << properties.albedo.x << properties.albedo.y << properties.albedo.z << properties.albedo.w << YAML::EndSeq;
        out << YAML::Key << "metallic" << YAML::Value << properties.metallic;
        out << YAML::Key << "roughness" << YAML::Value << properties.roughness;
        out << YAML::Key << "ambientOcclusion" << YAML::Value << properties.ambientOcclusion;
        out << YAML::EndMap;
        std::filesystem::path path = filepath;
        create_directories(path.parent_path());
        std::ofstream fout(filepath);
        fout << out.c_str();
    }


    bool MaterialManager::registerMaterial(const std::string& filepath) {
        YAML::Node data;
        try {
            data = YAML::LoadFile(filepath);
        } catch (YAML::Exception& e) {
            TE_LOGGER_CRITICAL("Failed to load material file: {0}", e.what());
            return false;
        }
        YAML::Node name = data["name"];
        YAML::Node albedoNode = data["albedo"];
        YAML::Node metallicNode = data["metallic"];
        YAML::Node roughnessNode = data["roughness"];
        YAML::Node ambientOcclusionNode = data["ambientOcclusion"];
        if (!name) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Name \"", filepath);
            return false;
        } else if (!albedoNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Color \"", filepath);
            return false;
        } else if (!metallicNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Ambient \"", filepath);
            return false;
        } else if (!roughnessNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Diffuse \"", filepath);
            return false;
        } else if (!ambientOcclusionNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Specular \"", filepath);
            return false;
        }
        glm::vec4 color = albedoNode.as<glm::vec4>();
        float ambient = ambientOcclusionNode.as<float>();
        float diffuse = metallicNode.as<float>();
        float specular = roughnessNode.as<float>();

        createMaterial(name.as<std::string>(), color);

        return true;
    }
}
