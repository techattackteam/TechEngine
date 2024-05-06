#include "MaterialManager.hpp"
#include "core/Logger.hpp"
#include "texture/TextureManager.hpp"
#include <utils/YAMLUtils.hpp>
#include <filesystem>
#include <fstream>


namespace TechEngine {
    MaterialManager::MaterialManager(EventDispatcher& eventDispatcher,
                                     TextureManager& textureManager) : eventDispatcher(eventDispatcher),
                                                                       m_textureManager(textureManager) {
    }

    void MaterialManager::init(const std::vector<std::string>& materialsFilePaths) {
        for (const std::string& materialFilePath: materialsFilePaths) {
            deserializeMaterial(materialFilePath);
        }
    }

    Material& MaterialManager::createMaterial(const std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) {
        Material material(eventDispatcher, name, color, ambient, diffuse, specular, shininess);
        auto iterator = m_materialsBank.find(name);
        if (iterator == m_materialsBank.end()) {
            iterator = m_materialsBank.emplace(name, material).first;
        }
        return iterator->second;
    }

    Material& MaterialManager::createMaterial(const std::string& name, Texture* diffuse) {
        Material material(eventDispatcher, name, diffuse);
        auto iterator = m_materialsBank.find(name);
        if (iterator == m_materialsBank.end()) {
            iterator = m_materialsBank.emplace(name, material).first;
        }
        return iterator->second;
    }

    Material& MaterialManager::createMaterialFile(const std::string& name, const std::string& folderPath) {
        std::string filepath = folderPath + "/" + name + ".mat";
        Material& material = createMaterial(name, m_defaultColor, m_defaultAmbient, m_defaultDiffuse, m_defaultSpecular, m_defaultShininess);
        serializeMaterial(name, filepath);
        return material;
    }

    bool MaterialManager::deleteMaterial(const std::string& name) {
        if (!m_materialsBank.empty() && materialExists(name)) {
            m_materialsBank.erase(name);
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
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << name;
        out << YAML::Key << "color" << YAML::Value << YAML::Flow << YAML::BeginSeq << material.getColor().x << material.getColor().y << material.getColor().z << material.getColor().w << YAML::EndSeq;
        out << YAML::Key << "ambient" << YAML::Value << YAML::Flow << YAML::BeginSeq << material.getAmbient().x << material.getAmbient().y << material.getAmbient().z << YAML::EndSeq;
        out << YAML::Key << "diffuse" << YAML::Value << YAML::Flow << YAML::BeginSeq << material.getDiffuse().x << material.getDiffuse().y << material.getDiffuse().z << YAML::EndSeq;
        out << YAML::Key << "specular" << YAML::Value << YAML::Flow << YAML::BeginSeq << material.getSpecular().x << material.getSpecular().y << material.getSpecular().z << YAML::EndSeq;
        out << YAML::Key << "shininess" << YAML::Value << material.getShininess();
        out << YAML::Key << "useTexture" << YAML::Value << material.getUseTexture();
        if (material.getUseTexture()) {
            out << YAML::Key << "diffuseTexture" << YAML::Value << material.getDiffuseTexture()->getName();
        } else {
            out << YAML::Key << "diffuseTexture" << YAML::Value << "";
        }
        out << YAML::EndMap;
        std::filesystem::path path = filepath;
        create_directories(path.parent_path());
        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    void MaterialManager::clear() {
        m_materialsBank.clear();
    }

    void MaterialManager::copy() {
        delete m_copy;
        m_copy = new MaterialManager(eventDispatcher, m_textureManager);
        //m_copy->m_materialsBank = m_materialsBank;
    }

    void MaterialManager::restoreCopy() {
        //m_materialsBank = m_copy->m_materialsBank;
        delete m_copy;
        m_copy = nullptr;
    }

    bool MaterialManager::deserializeMaterial(const std::string& filepath) {
        YAML::Node data;
        try {
            data = YAML::LoadFile(filepath);
        } catch (YAML::Exception& e) {
            TE_LOGGER_CRITICAL("Failed to load material file: {0}", e.what());
            return false;
        }
        YAML::Node name = data["name"];
        YAML::Node colorNode = data["color"];
        YAML::Node ambientNode = data["ambient"];
        YAML::Node diffuseNode = data["diffuse"];
        YAML::Node specularNode = data["specular"];
        YAML::Node shininessNode = data["shininess"];
        YAML::Node useTextureNode = data["useTexture"];
        YAML::Node diffuseTextureNode = data["diffuseTexture"];
        if (!name) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Name \"", filepath);
            return false;
        } else if (!colorNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Color \"", filepath);
            return false;
        } else if (!ambientNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Ambient \"", filepath);
            return false;
        } else if (!diffuseNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Diffuse \"", filepath);
            return false;
        } else if (!specularNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Specular \"", filepath);
            return false;
        } else if (!shininessNode) {
            TE_LOGGER_CRITICAL("Failed to load material file: {0}", filepath);
            return false;
        } else if (!useTextureNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Use Texture \"", filepath);
            return false;
        } else if (useTextureNode && useTextureNode.as<bool>() && !diffuseTextureNode) {
            TE_LOGGER_CRITICAL("Failed to load material file {0} - \"Invalid Diffuse Texture \"", filepath);
            return false;
        }

        glm::vec4 color = colorNode.as<glm::vec4>();
        glm::vec3 ambient = ambientNode.as<glm::vec3>();
        glm::vec3 diffuse = diffuseNode.as<glm::vec3>();
        glm::vec3 specular = specularNode.as<glm::vec3>();
        float shininess = shininessNode.as<float>();
        Material& material = createMaterial(name.as<std::string>(), color, ambient, diffuse, specular, shininess);
        material.setUseTexture(useTextureNode.as<bool>());
        std::string diffuseTextureName = diffuseTextureNode.as<std::string>();
        if (!diffuseTextureName.empty()) {
            Texture* diffuseTexture = &m_textureManager.getTexture(diffuseTextureName);
            material.setDiffuseTexture(diffuseTexture);
        } else {
            material.setDiffuseTexture(nullptr);
            material.setUseTexture(false);
        }
        return true;
    }
}
