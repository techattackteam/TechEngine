#include <glm/gtc/type_ptr.hpp>
#include "TransformComponent.hpp"
#include "../../../lib/imgui/imgui_internal.h"

namespace Engine {
    TransformComponent::TransformComponent(const std::string &gameObjectName)
            : position(glm::vec3(0, 0, 0)), rotation(glm::vec3(0, 0, 0)), scale(glm::vec3(1, 1, 1)), Component("Transform") {
        this->gameObjectName = gameObjectName;
    }

    glm::mat4 TransformComponent::getModelMatrix() {
        glm::mat4 transform = glm::translate(glm::mat4(1), position);
        //rotation;
        glm::mat4 scale = glm::scale(glm::mat4(1), this->scale);
        return scale * transform;
    }

    glm::vec3 TransformComponent::getPosition() {
        return position;
    }

    void TransformComponent::setPosition(glm::vec3 vec1) {
        position = vec1;
    }

    void TransformComponent::drawDrag(const std::string &name, glm::vec3 &value) {
        ImGui::Text("%s", name.c_str());
        ImGui::PushID(name.c_str());
        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        ImGui::AlignTextToFramePadding();
        ImGui::Text("X ");
        ImGui::SameLine();
        ImGui::DragFloat("##X", &value.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::Text(" Y ");
        ImGui::SameLine();
        ImGui::DragFloat("##Y", &value.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::Text(" Z ");
        ImGui::SameLine();
        ImGui::DragFloat("##Z", &value.z, 0.1f, 0.0f, 0.0f, "%.2f");

        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
        ImGui::PopID();
    }

    void TransformComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {
            drawDrag("Position", position);
            drawDrag("Rotation", rotation);
            drawDrag("Scale", scale);
        }
    }

}

