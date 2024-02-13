#include "MeshRendererComponent.hpp"
#include "components/MeshRendererComponent.hpp"

#include "core/Logger.hpp"
#include "mesh/CubeMesh.hpp"
#include "mesh/CylinderMesh.hpp"
#include "mesh/SphereMesh.hpp"

#include "ScriptingAPI/material/MaterialManagerAPI.hpp"

namespace TechEngineAPI {
    MeshRendererComponent::MeshRendererComponent(TechEngine::MeshRendererComponent* meshRendererComponent) {
        this->meshRendererComponent = meshRendererComponent;
    }

    void MeshRendererComponent::changeMesh(MeshType meshType) {
        delete &this->meshRendererComponent->getMesh();
        switch (meshType) {
            case MeshType::Cube:
                this->meshRendererComponent->changeMesh(new TechEngine::CubeMesh());
                break;
            case MeshType::Sphere:
                this->meshRendererComponent->changeMesh(new TechEngine::SphereMesh());
                break;
            case MeshType::Cylinder:
                this->meshRendererComponent->changeMesh(new TechEngine::CylinderMesh());
                break;
            case MeshType::Custom:
                TE_LOGGER_WARN("Custom mesh file not included!");
                break;
        }
    }

    void MeshRendererComponent::changeMesh(MeshType meshType, std::string& filePath) {
        delete &this->meshRendererComponent->getMesh();
        switch (meshType) {
            case MeshType::Cube:
                this->meshRendererComponent->changeMesh(new TechEngine::CubeMesh());
                break;
            case MeshType::Sphere:
                this->meshRendererComponent->changeMesh(new TechEngine::SphereMesh());
                break;
            case MeshType::Cylinder:
                this->meshRendererComponent->changeMesh(new TechEngine::CylinderMesh());
                break;
            case MeshType::Custom:
                TE_LOGGER_WARN("Custom mesh in script not implemented!");
                break;
        }
    }

    void MeshRendererComponent::changeMaterial(std::shared_ptr<Material> material) {
        this->meshRendererComponent->changeMaterial(*material->material);
    }

    std::shared_ptr<Material> MeshRendererComponent::getMaterial() {
        return material;
    }
}
