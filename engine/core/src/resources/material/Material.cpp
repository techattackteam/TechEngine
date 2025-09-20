#include "Material.hpp"

#include <glm/glm.hpp>

namespace TechEngine {
    Material::Material(std::string name,
                       uint32_t gpuID,
                       glm::vec4 color,
                       glm::vec3 ambient,
                       glm::vec3 diffuse,
                       glm::vec3 specular,
                       float shininess) : name(std::move(name)),
                                          m_gpuID(gpuID),
                                          properties(color),
                                          ambient(ambient),
                                          diffuse(diffuse),
                                          specular(specular),
                                          shininess(shininess) {
    }


    std::string& Material::getName() {
        return name;
    }

    uint32_t Material::getGpuID() const {
        return m_gpuID;
    }

    glm::vec4& Material::getColor() {
        return properties.color;
    }

    void Material::setColor(glm::vec4 color) {
        this->properties.color = color;
    }

    glm::vec3 Material::getAmbient() {
        return ambient;
    }

    void Material::setAmbient(glm::vec3 ambient) {
        this->ambient = ambient;
    }

    glm::vec3 Material::getDiffuse() {
        return diffuse;
    }

    void Material::setDiffuse(glm::vec3 diffuse) {
        this->diffuse = diffuse;
    }

    glm::vec3 Material::getSpecular() {
        return specular;
    }

    void Material::setSpecular(glm::vec3 specular) {
        this->specular = specular;
    }

    float Material::getShininess() {
        return shininess;
    }

    void Material::setShininess(float shininess) {
        this->shininess = shininess;
    }

    MaterialProperties& Material::getProperties() {
        return properties;
    }
}
