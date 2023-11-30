#include "MeshRendererComponent.hpp"
#include "mesh/CubeMesh.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    MeshRendererComponent::MeshRendererComponent(GameObject *gameObject) :
            mesh(new CubeMesh()), m_material(), Component(gameObject, "MeshRenderer") {

    }

    MeshRendererComponent::MeshRendererComponent(GameObject *gameObject, Mesh *mesh, Material *material) :
            mesh(mesh), m_material(material), Component(gameObject, "MeshRenderer") {
        paintMesh();
    }

    void MeshRendererComponent::changeMesh(Mesh *mesh) {
        this->mesh = mesh;
        paintMesh();
    }

    void MeshRendererComponent::changeMaterial(Material &material) {
        this->m_material = &material;
        paintMesh();
    }

    void MeshRendererComponent::paintMesh() {
        for (Vertex &vertex: mesh->getVertices()) {
            vertex.setColor(m_material->getColor());
        }
    }

    Mesh &MeshRendererComponent::getMesh() {
        return *mesh;
    }

    Material &MeshRendererComponent::getMaterial() {
        return *m_material;
    }

    std::vector<Vertex> MeshRendererComponent::getVertices() {
        return mesh->getVertices();
    }

    std::vector<int> MeshRendererComponent::getIndices() {
        return mesh->getIndices();
    }


}
