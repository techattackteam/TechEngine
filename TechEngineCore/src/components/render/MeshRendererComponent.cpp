#include "MeshRendererComponent.hpp"

#include "mesh/MeshManager.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    MeshRendererComponent::MeshRendererComponent(GameObject* gameObject,
                                                 SystemsRegistry& systemsRegistry) : mesh(systemsRegistry.getSystem<MeshManager>().getMesh("Cube")),
                                                                                     Component(gameObject, systemsRegistry, "MeshRenderer") {
        std::string name = "DefaultMaterial";
        m_material = &systemsRegistry.getSystem<MaterialManager>().getMaterial(name);
        paintMesh();
    }

    MeshRendererComponent::MeshRendererComponent(GameObject* gameObject,
                                                 SystemsRegistry& systemsRegistry,
                                                 Mesh& mesh,
                                                 Material* material) : mesh(mesh),
                                                                       m_material(material),
                                                                       Component(gameObject, systemsRegistry, "MeshRenderer") {
        paintMesh();
    }

    void MeshRendererComponent::changeMesh(Mesh& mesh) {
        this->mesh = mesh;
        if (m_material != nullptr)
            paintMesh();
    }

    void MeshRendererComponent::changeMaterial(Material& material) {
        this->m_material = &material;
        paintMesh();
    }

    void MeshRendererComponent::changeMaterial(std::string& name) {
        paintMesh();
    }


    void MeshRendererComponent::paintMesh() {
        for (Vertex& vertex: mesh.getVertices()) {
            vertex.setColor(m_material->getColor());
        }
    }

    Mesh& MeshRendererComponent::getMesh() {
        return mesh;
    }

    Material& MeshRendererComponent::getMaterial() {
        return *m_material;
    }

    std::vector<Vertex> MeshRendererComponent::getVertices() {
        return mesh.getVertices();
    }

    std::vector<int> MeshRendererComponent::getIndices() {
        return mesh.getIndices();
    }

    Component* MeshRendererComponent::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        MeshRendererComponent* meshRenderer = (MeshRendererComponent*)componentToCopy;
        Mesh& mesh = *new Mesh(meshRenderer->getVertices(), meshRenderer->getIndices());
        auto* newComponent = new MeshRendererComponent(gameObjectToAttach, systemsRegistry, mesh, m_material);
        return newComponent;
    }

    void MeshRendererComponent::Serialize(StreamWriter* stream) {
        Component::Serialize(stream);
        stream->writeString(mesh.getName());
        stream->writeString(m_material->getName());
    }

    void MeshRendererComponent::Deserialize(StreamReader* stream) {
        Component::Deserialize(stream);
        std::string meshName;
        std::string materialName;
        stream->readString(meshName);
        stream->readString(materialName);
        TE_LOGGER_INFO("MeshRendererComponent::Deserialize: meshName: " + meshName + ", materialName: " + materialName);
        mesh = systemsRegistry.getSystem<MeshManager>().getMesh(meshName);
        m_material = &systemsRegistry.getSystem<MaterialManager>().getMaterial(materialName);
        paintMesh();
    }
}
