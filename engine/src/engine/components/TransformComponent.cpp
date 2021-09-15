#include <glm/gtc/type_ptr.hpp>
#include "TransformComponent.hpp"

namespace Engine {
    TransformComponent::TransformComponent() : position(glm::vec3(0, 0, 0)), rotation(glm::vec3(0, 0, 0)), scale(glm::vec3(1, 1, 1)), Component("Transform") {
    }

    glm::mat4 TransformComponent::getModelMatrix() {
        glm::mat4 transform = glm::translate(glm::mat4(1), position);
        //rotation;
        glm::mat4 scale = glm::scale(glm::mat4(1), this->scale);
        return scale * transform;
    }

    void TransformComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {
            ImGui::Text("   Position:");
            ImGui::Text("   x: %0.00f y: %0.00f z: %.2f", position.x, position.y, position.z);
            ImGui::Text("   Scale:");
            ImGui::Text("   x: %0.00f y: %0.00f z: %.2f", scale.x, scale.y, scale.z);
        }
    }

}

