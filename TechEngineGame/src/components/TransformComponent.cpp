#include "TransformComponent.hpp"
#include "imgui_internal.h"
#include <iostream>

namespace TechEngine {

    TransformComponent::TransformComponent(GameObject *gameObject) : Transform(gameObject) {

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