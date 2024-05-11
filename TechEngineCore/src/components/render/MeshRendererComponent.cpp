#include "MeshRendererComponent.hpp"
#include "mesh/CubeMesh.hpp"
#include "scriptingAPI/material/MaterialManagerAPI.hpp"

namespace TechEngine {
    MeshRendererComponent::MeshRendererComponent(GameObject* gameObject, EventDispatcher& eventDispatcher) : mesh(new CubeMesh()), Component(gameObject, eventDispatcher, "MeshRenderer") {
        std::string name = "DefaultMaterial";
        m_material = MaterialManagerAPI::getMaterial(name);
        paintMesh();
    }

    MeshRendererComponent::MeshRendererComponent(GameObject* gameObject,
                                                 EventDispatcher& eventDispatcher,
                                                 Mesh* mesh,
                                                 Material* material) : mesh(mesh),
                                                                       m_material(material),
                                                                       Component(gameObject, eventDispatcher, "MeshRenderer") {
        paintMesh();
    }

    void MeshRendererComponent::changeMesh(Mesh* mesh) {
        this->mesh = mesh;
        if (m_material != nullptr)
            paintMesh();
    }

    void MeshRendererComponent::changeMaterial(Material& material) {
        this->m_material = &material;
        paintMesh();
    }

    void MeshRendererComponent::changeMaterial(std::string& name) {
        m_material = MaterialManagerAPI::getMaterial(name);
        paintMesh();
    }


    void MeshRendererComponent::paintMesh() {
        for (Vertex& vertex: mesh->getVertices()) {
            vertex.setColor(m_material->getColor());
        }
    }

    Mesh& MeshRendererComponent::getMesh() {
        return *mesh;
    }

    Material& MeshRendererComponent::getMaterial() {
        return *m_material;
    }

    std::vector<Vertex> MeshRendererComponent::getVertices() {
        return mesh->getVertices();
    }

    std::vector<int> MeshRendererComponent::getIndices() {
        return mesh->getIndices();
    }

    Component* MeshRendererComponent::copy(GameObject* gameObjectToAttach, Component* componentToCopy) {
        MeshRendererComponent* meshRenderer = (MeshRendererComponent*)componentToCopy;
        Mesh* mesh = new Mesh(meshRenderer->getVertices(), meshRenderer->getIndices());
        auto* newComponent = new MeshRendererComponent(gameObjectToAttach, eventDispatcher, mesh, m_material);
        return newComponent;
    }

    void MeshRendererComponent::Serialize(StreamWriter* stream) {
        Component::Serialize(stream);
        //stream->writeString(mesh->getName());
        stream->writeString(m_material->getName());
    }

    void MeshRendererComponent::Deserialize(StreamReader* stream) {
        Component::Deserialize(stream);
        //mesh = new CubeMesh();
        std::string materialName;
        stream->readString(materialName);
        m_material = MaterialManagerAPI::getMaterial(materialName);
    }
}
