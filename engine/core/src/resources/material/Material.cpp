#include "TechEngine/core/resources/material/Material.hpp"

#include <glm/glm.hpp>

namespace TechEngine {
    Material::Material(std::string name,
                       uint32_t gpuID,
                       glm::vec4 color) : name(std::move(name)),
                                          m_gpuID(gpuID),
                                          properties() {
    }


    std::string& Material::getName() {
        return name;
    }

    uint32_t Material::getGpuID() const {
        return m_gpuID;
    }

    glm::vec4& Material::getColor() {
        return properties.albedo;
    }

    void Material::setColor(glm::vec4 color) {
        this->properties.albedo = color;
    }

    MaterialProperties& Material::getProperties() {
        return properties;
    }

    int& Material::getAlbedoMapID() {
        return albedoMapID;
    }

    int& Material::getNormalMapID() {
        return normalMapID;
    }

    int& Material::getMetallicMapID() {
        return metallicMapID;
    }

    int& Material::getRoughnessMapID() {
        return roughnessMapID;
    }
}
