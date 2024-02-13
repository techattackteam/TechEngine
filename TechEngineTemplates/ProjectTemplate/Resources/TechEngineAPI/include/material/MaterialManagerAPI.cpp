#include "MaterialManagerAPI.hpp"

#include "core/Logger.hpp"
#include "material/MaterialManager.hpp"
#include "material/Material.hpp"

namespace TechEngineAPI {
    MaterialManagerAPI::MaterialManagerAPI(TechEngine::MaterialManager* materialManager) {
        MaterialManagerAPI::materialManager = materialManager;
    }

    std::shared_ptr<Material> MaterialManagerAPI::createMaterial(std::string& name) {
        TechEngine::Material& material = materialManager->getMaterial("DefaultMaterial");
        return copyMaterial(name, &material);
    }

    std::shared_ptr<Material> MaterialManagerAPI::copyMaterial(std::string& name, const std::shared_ptr<Material>& basedMaterial) {
        TechEngine::Material* material = basedMaterial->material;
        return copyMaterial(name, material);
    }

    std::shared_ptr<Material> MaterialManagerAPI::copyMaterial(std::string& name, TechEngine::Material* material) {
        glm::vec4 color = material->getColor();
        glm::vec3 ambient = material->getAmbient();
        glm::vec3 diffuse = material->getDiffuse();
        glm::vec3 specular = material->getSpecular();
        float shininess = material->getShininess();
        return createMaterial(name, color, ambient, diffuse, specular, shininess);
    }

    std::shared_ptr<Material> MaterialManagerAPI::createMaterial(std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) {
        if (materialManager->materialExists(name)) {
            TE_LOGGER_WARN("Material {0} already exists!", name);
            return nullptr;
        }
        TechEngine::Material& newMaterial = materialManager->createMaterial(name, color, ambient, diffuse, specular, shininess);
        return std::make_shared<Material>(&newMaterial);
    }

    bool MaterialManagerAPI::deleteMaterial(std::string& name) {
        return materialManager->deleteMaterial(name);
    }

    std::shared_ptr<Material> MaterialManagerAPI::getMaterial(std::string& name) {
        return std::make_shared<Material>(&materialManager->getMaterial(name));
    }
}
