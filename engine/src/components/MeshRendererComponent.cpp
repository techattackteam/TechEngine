#include "MeshRendererComponent.hpp"


namespace Engine {

    MeshRendererComponent::MeshRendererComponent(GameObject *gameObject, Mesh *mesh) : mesh(*mesh), Component(gameObject, "MeshRenderer") {
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
