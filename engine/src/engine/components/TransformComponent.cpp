#include "TransformComponent.hpp"
#include <iostream>

namespace Engine {
    TransformComponent::TransformComponent(GameObject *gameObject)
            : position(glm::vec3(0, 0, 0)), orientation(glm::vec3(0, 0, 0)), scale(glm::vec3(1, 1, 1)), model(glm::mat4(1.0f)), Component(gameObject, "Transform") {
    }

    glm::mat4 TransformComponent::getModelMatrix() {
        model = glm::translate(glm::mat4(1), position) * glm::toMat4(glm::quat(glm::radians(orientation))) * glm::scale(glm::mat4(1), scale);
        return model;
    }

    void TransformComponent::translate(glm::vec3 vector) {
        position += vector;
    }

    void TransformComponent::translateTo(glm::vec3 position) {
        translate(position - this->position);
        this->position = position;
    }

    /*Rotate using Euler angles in Degrees*/
    void TransformComponent::rotate(glm::vec3 rotation) {
        this->orientation = rotation;
    }

    /*Rotate using Quaternions in Degrees*/
    void TransformComponent::rotate(glm::quat quaternion) {
        rotate(glm::degrees(glm::eulerAngles(quaternion)));
    }

    void TransformComponent::setScale(glm::vec3 scale) {
        this->scale = scale;
    }

    void TransformComponent::lookAt(glm::vec3 lookPosition) {
        glm::mat4 lootAtMatrix = glm::lookAt(position, lookPosition, glm::vec3(0, 1, 0));
        rotate(glm::toQuat(lootAtMatrix));
    }

    glm::vec3 TransformComponent::getPosition() const {
        return position;
    }

    glm::vec3 TransformComponent::getOrientation() const {
        return orientation;
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
            glm::vec3 newPosition = position;
            drawDrag("Position", newPosition);
            if (newPosition != position) {
                translateTo(newPosition);
            }

            glm::vec3 newOrientation = orientation;
            drawDrag("Rotation", newOrientation);
            if (newOrientation != orientation) {
                rotate(newOrientation);
            }

            glm::vec3 newScale = scale;
            drawDrag("Scale", newScale);
            if (newScale != scale) {
                setScale(newScale);
            }
        }
    }
}

