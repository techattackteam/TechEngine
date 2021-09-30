#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include "CameraComponent.hpp"
#include "TransformComponent.hpp"

namespace Engine {
    CameraComponent::CameraComponent(GameObject *gameObject) : Component("Camera") {
        this->gameObject = gameObject;
        mainCamera = false;
        update();
    }

    CameraComponent::CameraComponent(GameObject *gameObject, bool mainCamera) : Component("Camera") {
        this->gameObject = gameObject;
        this->mainCamera = mainCamera;
        update();
    }

    void CameraComponent::update() {
        updateViewMatrix();
        updateProjectionMatrix();
    }

    void CameraComponent::updateProjectionMatrix() {
        projectionMatrix = glm::perspective(glm::radians(fov), 16.0f / 9.0f, 0.1f, 50.0f);
    }

    void CameraComponent::updateViewMatrix() {
        //TEMP FIX
        gameObject->getComponent<TransformComponent>()->setScale(glm::vec3(1, 1, 1));
        glm::mat4 model = gameObject->getComponent<TransformComponent>()->getModelMatrix();
        viewMatrix = glm::inverse(model);
    }

    glm::mat4 Engine::CameraComponent::getViewMatrix() {
        return viewMatrix;
    }

    glm::mat4 CameraComponent::getProjectionMatrix() {
        return projectionMatrix;
    }

    void CameraComponent::getInfo() {
        if (ImGui::CollapsingHeader(name.c_str())) {
            ImGui::PushID(name.c_str());
            ImGui::PushMultiItemsWidths(1, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

            ImGui::Text(" Fov ");
            ImGui::SameLine();
            ImGui::DragFloat("##FOV", &fov, 0.1f, 45.0f, 120.0f, "%.2f");
            ImGui::NewLine();
            ImGui::PopItemWidth();
            ImGui::PopItemWidth();
            ImGui::PopStyleVar();
            ImGui::PopID();
        }

    }

    bool CameraComponent::isMainCamera() {
        return mainCamera;
    }


}
