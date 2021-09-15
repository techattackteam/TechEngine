#include "MeshComponent.hpp"

namespace Engine {

    MeshComponent::MeshComponent() : Component("Mesh") {

    }

    void MeshComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {

        }
    }

}
