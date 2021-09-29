#include "MeshRendererComponent.hpp"


namespace Engine {

    MeshRendererComponent::MeshRendererComponent(Mesh *mesh) : mesh(*mesh), Component("MeshRenderer") {
    }

    void MeshRendererComponent::changeMesh(Mesh &mesh) {
        this->mesh = mesh;
    }

    void MeshRendererComponent::renderMesh() {

    }

    void MeshRendererComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {

        }
    }

    Mesh &MeshRendererComponent::getMesh() {
        return mesh;
    }
}
