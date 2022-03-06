#include "MeshRendererComponent.hpp"


namespace Engine {

    MeshRendererComponent::MeshRendererComponent(GameObject *gameObject, Mesh *mesh, Material *material) : mesh(*mesh), material(*material), Component(gameObject, "MeshRenderer") {
    }

    void MeshRendererComponent::changeMesh(Mesh &mesh) {
        this->mesh = mesh;
    }

    void MeshRendererComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {

        }
    }

    Mesh &MeshRendererComponent::getMesh() {
        return mesh;
    }

    Material &MeshRendererComponent::getMaterial() {
        return material;
    }
}
