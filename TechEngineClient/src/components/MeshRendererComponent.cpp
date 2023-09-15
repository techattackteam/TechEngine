#include "MeshRendererComponent.hpp"
#include "mesh/CubeMesh.hpp"


namespace TechEngine {
    MeshRendererComponent::MeshRendererComponent(GameObject *gameObject) :
            mesh(new CubeMesh()), material(*new Material(glm::vec4(1, 1, 1, 1), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 32.0f)), Component(gameObject, "MeshRenderer") {

    }

    MeshRendererComponent::MeshRendererComponent(GameObject *gameObject, Mesh *mesh, Material *material) :
            mesh(mesh), material(*material), Component(gameObject, "MeshRenderer") {
        paintMesh();
    }

    void MeshRendererComponent::changeMesh(Mesh *mesh) {
        this->mesh = mesh;
        paintMesh();
    }

    void MeshRendererComponent::paintMesh() {
        for (Vertex &vertex: mesh->getVertices()) {
            vertex.color = material.getColor();
        }
    }

    Mesh &MeshRendererComponent::getMesh() {
        return *mesh;
    }

    Material &MeshRendererComponent::getMaterial() {
        return material;
    }

    std::vector<Vertex> MeshRendererComponent::getVertices() {
        return mesh->getVertices();
    }

    std::vector<int> MeshRendererComponent::getIndices() {
        return mesh->getIndices();
    }
}
