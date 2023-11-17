#include "MaterialManager.hpp"
#include "core/Logger.hpp"
#include "renderer/TextureManager.hpp"
#include <utils/YAMLUtils.hpp>
#include <filesystem>
#include <fstream>

namespace TechEngine {

    void MaterialManager::init(const std::vector<std::string> &materialsFilePaths) {
        for (const std::string &materialFilePath: materialsFilePaths) {
            deserializeMaterial(materialFilePath);
        }
    }

    Material &MaterialManager::createMaterial(const std::string &name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) {
        Material material(name, color, ambient, diffuse, specular, shininess);
        auto iterator = getInstance().m_materialsBank.find(name);
        if (iterator == getInstance().m_materialsBank.end()) {
            iterator = getInstance().m_materialsBank.emplace(name, material).first;
        }
        return iterator->second;
    }

    Material &MaterialManager::createMaterial(const std::string &name, Texture *diffuse) {
        Material material(name, diffuse);
        auto iterator = getInstance().m_materialsBank.find(name);
        if (iterator == getInstance().m_materialsBank.end()) {
            iterator = getInstance().m_materialsBank.emplace(name, material).first;
        }
        return iterator->second;
    }

    Material &MaterialManager::createMaterialFile(const std::string &name, const std::string &folderPath) {
        std::string filepath = folderPath + "/" + name + ".mat";
        Material &material = createMaterial(name, getInstance().m_defaultColor, getInstance().m_defaultAmbient, getInstance().m_defaultDiffuse, getInstance().m_defaultSpecular, getInstance().m_defaultShininess);
        serializeMaterial(name, filepath);
        return material;
    }

    bool MaterialManager::deleteMaterial(const std::string &name) {
        if (!getInstance().m_materialsBank.empty() && materialExists(name)) {
            getInstance().m_materialsBank.erase(name);
            return true;
        }
        return false;
    }

    bool MaterialManager::materialExists(const std::string &name) {
        return getInstance().m_materialsBank.contains(name);
    }

    Material &MaterialManager::getMaterial(const std::string &name) {
        if (!materialExists(name)) {
            TE_LOGGER_ERROR("Material {0} not found", name);
        }
        return getInstance().m_materialsBank.at(name);
    }

    void MaterialManager::serializeMaterial(const std::string &name, const std::string &filepath) {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << name;
        out << YAML::Key << "color" << YAML::Value << YAML::Flow << YAML::BeginSeq << getMaterial(name).getColor().x << getMaterial(name).getColor().y << getMaterial(name).getColor().z << getMaterial(name).getColor().w << YAML::EndSeq;
        out << YAML::Key << "ambient" << YAML::Value << YAML::Flow << YAML::BeginSeq << getMaterial(name).getAmbient().x << getMaterial(name).getAmbient().y << getMaterial(name).getAmbient().z << YAML::EndSeq;
        out << YAML::Key << "diffuse" << YAML::Value << YAML::Flow << YAML::BeginSeq << getMaterial(name).getDiffuse().x << getMaterial(name).getDiffuse().y << getMaterial(name).getDiffuse().z << YAML::EndSeq;
        out << YAML::Key << "specular" << YAML::Value << YAML::Flow << YAML::BeginSeq << getMaterial(name).getSpecular().x << getMaterial(name).getSpecular().y << getMaterial(name).getSpecular().z << YAML::EndSeq;
        out << YAML::Key << "shininess" << YAML::Value << getMaterial(name).getShininess();
        out << YAML::Key << "useTexture" << YAML::Value << getMaterial(name).getUseTexture();
        if (getMaterial(name).getUseTexture()) {
            out << YAML::Key << "diffuseTexture" << YAML::Value << getMaterial(name).getDiffuseTexture()->getName();
        } else {
            out << YAML::Key << "diffuseTexture" << YAML::Value << "";
        }
        out << YAML::EndMap;
        std::filesystem::path path = filepath;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    bool MaterialManager::deserializeMaterial(const std::string &filepath) {
        YAML::Node data;
        try {
            data = YAML::LoadFile(filepath);
        } catch (YAML::Exception &e) {
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
        Material &material = createMaterial(name.as<std::string>(), color, ambient, diffuse, specular, shininess);
        material.getUseTexture() = useTextureNode.as<bool>();
        std::string diffuseTextureName = diffuseTextureNode.as<std::string>();
        if (!diffuseTextureName.empty()) {
            Texture *diffuseTexture = &TextureManager::getTexture(diffuseTextureName);
            material.setDiffuseTexture(diffuseTexture);
        } else {
            material.setDiffuseTexture(nullptr);
            material.getUseTexture() = false;
        }
        return true;
    }


}