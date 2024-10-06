#include "resources/Material.hpp"

#include "resources/ResourcesManager.hpp"

namespace TechEngineAPI {
    Material::Material(const std::string& name, TechEngine::ResourcesManager* resourcesManager) : m_name(name), m_resourcesManager(resourcesManager) {
    }

    const std::string& Material::getName() const {
        return m_name;
    }

    const glm::vec4& Material::getColor() const {
        return m_color;
    }

    const glm::vec3& Material::getAmbient() const {
        return m_ambient;
    }

    const glm::vec3& Material::getDiffuse() const {
        return m_diffuse;
    }

    const glm::vec3& Material::getSpecular() const {
        return m_specular;
    }

    const float Material::getShininess() const {
        return m_shininess;
    }

    void Material::setColor(const glm::vec4& color) {
        m_color = color;
        m_resourcesManager->getMaterial(m_name).setColor(color);
    }

    void Material::setAmbient(const glm::vec3& ambient) {
        m_ambient = ambient;
    }

    void Material::setDiffuse(const glm::vec3& diffuse) {
        m_diffuse = diffuse;
    }

    void Material::setSpecular(const glm::vec3& specular) {
        m_specular = specular;
    }
}
