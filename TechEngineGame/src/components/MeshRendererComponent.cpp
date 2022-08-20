#include "MeshRendererComponent.hpp"


namespace TechEngine {

    MeshRendererComponent::MeshRendererComponent(GameObject *gameObject, Mesh *mesh, Material *material) :
            mesh(*mesh), material(*material), Component(gameObject, "MeshRenderer") {
        paintMesh();
    }

    void MeshRendererComponent::changeMesh(Mesh &mesh) {
        this->mesh = mesh;
        paintMesh();
    }

    void MeshRendererComponent::paintMesh() {
        for (Vertex &vertex: mesh.getVertices()) {
            vertex.color = material.getColor();
        }
    }

    void MeshRendererComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {

        }
    }

    Material &MeshRendererComponent::getMaterial() {
        return material;
    }

    std::vector<Vertex> MeshRendererComponent::getVertices() {
        return mesh.getVertices();
    }
}
