#include "MaterialManagerAPI.hpp"

#include "core/Logger.hpp"
#include "material/MaterialManager.hpp"
#include "material/Material.hpp"

namespace TechEngine {
    MaterialManagerAPI::MaterialManagerAPI(MaterialManager* materialManager) {
        MaterialManagerAPI::materialManager = materialManager;
    }

    Material* MaterialManagerAPI::createMaterial(std::string& name) {
        Material& material = materialManager->getMaterial("DefaultMaterial");
        return copyMaterial(name, &material);
    }

    Material* MaterialManagerAPI::copyMaterial(std::string& name, Material* material) {
        glm::vec4 color = material->getColor();
        glm::vec3 ambient = material->getAmbient();
        glm::vec3 diffuse = material->getDiffuse();
        glm::vec3 specular = material->getSpecular();
        float shininess = material->getShininess();
        return createMaterial(name, color, ambient, diffuse, specular, shininess);
    }

    Material* MaterialManagerAPI::createMaterial(std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) {
        if (materialManager->materialExists(name)) {
            TE_LOGGER_WARN("Material {0} already exists!", name);
        }
        return &materialManager->createMaterial(name, color, ambient, diffuse, specular, shininess);
    }

    bool MaterialManagerAPI::deleteMaterial(std::string& name) {
        return materialManager->deleteMaterial(name);
    }

    Material* MaterialManagerAPI::getMaterial(std::string& name) {
        return &materialManager->getMaterial(name);
    }
}
