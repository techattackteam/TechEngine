#include "common/include/resources/Resources.hpp"
#include "common/include/resources/Mesh.hpp"

#include "resources/ResourcesManager.hpp"

namespace TechEngineAPI {
    void Resources::init(TechEngine::ResourcesManager* resourcesManager) {
        Resources::resourcesManager = resourcesManager;
    }

    std::shared_ptr<Mesh> Resources::createMesh(const std::string& name) {
        resourcesManager->createMesh(name);
        return std::shared_ptr<Mesh>(new Mesh(name, resourcesManager));
    }

    std::shared_ptr<Mesh> Resources::getMesh(const std::string& name) {
        resourcesManager->getMesh(name);
        return std::shared_ptr<Mesh>(new Mesh(name, resourcesManager));
    }

    std::shared_ptr<Material> Resources::createMaterial(const std::string& name) {
        TechEngine::Material& mat = resourcesManager->createMaterial(name);
        std::shared_ptr<Material> material = std::shared_ptr<Material>(new Material(name, resourcesManager));
        material->setColor(mat.getColor());
        material->setAmbient(mat.getAmbient());
        material->setDiffuse(mat.getDiffuse());
        material->setSpecular(mat.getSpecular());
        return material;
    }

    std::shared_ptr<Material> Resources::getMaterial(const std::string& name) {
        resourcesManager->getMaterial(name);
        return std::shared_ptr<Material>(new Material(name, resourcesManager));
    }
}
