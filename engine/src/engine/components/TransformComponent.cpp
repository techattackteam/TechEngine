#include "TransformComponent.hpp"
#include <cmath>
#include <iostream>

namespace Engine {
    TransformComponent::TransformComponent()
            : position(glm::vec3(0, 0, 0)), orientation(glm::vec3(0, 0, 0)), scale(glm::vec3(1, 1, 1)), model(glm::mat4(1.0f)), Component("Transform") {
    }

    glm::mat4 TransformComponent::getModelMatrix() const {
        return model;
    }

    void TransformComponent::translate(glm::vec3 vector) {
        position += vector;
        model = glm::translate(model, vector);
    }

    void TransformComponent::translateTo(glm::vec3 position) {
        translate(position - this->position);
        this->position = position;
    }

    void TransformComponent::rotate(glm::vec3 rotation) {
        glm::vec3 newRotation = rotation - this->orientation;
        model *= glm::toMat4(glm::quat(glm::radians(newRotation)));
        this->orientation = rotation;
    }

    void TransformComponent::setScale(glm::vec3 scale) {
        model[0][0] = scale[0];
        model[1][1] = scale[1];
        model[2][2] = scale[2];
        model[3][3] = 1;
        this->scale = scale;
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

            glm::vec3 newRotation = orientation;
            drawDrag("Rotation", newRotation);
            if (newRotation != orientation) {
                rotate(newRotation);
            }

            glm::vec3 newScale = scale;
            drawDrag("Scale", newScale);
            if (newScale != scale) {
                setScale(newScale);
            }
        }
    }
}

