#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include "CameraComponent.hpp"
#include "TransformComponent.hpp"
#include "imgui_internal.h"
#include "../event/EventDispatcher.hpp"
#include "../core/Window.hpp"

namespace TechEngine {
    CameraComponent::CameraComponent(GameObject *gameObject) : Component(gameObject, "Camera") {
        mainCamera = false;
        this->projectionType = PERSPECTIVE;
        init();
    }

    CameraComponent::CameraComponent(GameObject *gameObject, bool mainCamera, ProjectionType projectionType) : Component(gameObject, "Camera") {
        this->mainCamera = mainCamera;
        this->projectionType = projectionType;
        init();
    }

    void CameraComponent::init() {
        TechEngineCore::EventDispatcher::getInstance().subscribe(WindowResizeEvent::eventType, [this](TechEngineCore::Event *event) {
            onWindowResizeEvent((WindowResizeEvent *) (event));
        });

        updateViewMatrix();
        updateProjectionMatrix();
    }

    void CameraComponent::fixedUpdate() {
        updateViewMatrix();
    }

    void CameraComponent::updateProjectionMatrix() {
        if (projectionType == PERSPECTIVE) {
            projectionMatrix = glm::perspective(glm::radians(fov), WindowSettings::aspectRatio, 0.1f, 50.0f);
        } else if (projectionType == ORTHOGRAPHIC) {
            projectionMatrix = glm::ortho(-5.0f * WindowSettings::aspectRatio, 5.0f * WindowSettings::aspectRatio, -5.0f, 5.0f, 0.3f, 1000.0f);
        }
    }

    void CameraComponent::updateViewMatrix() {
        gameObject->getComponent<TransformComponent>()->setScale(glm::vec3(1, 1, 1));
        glm::mat4 model = gameObject->getComponent<TransformComponent>()->getModelMatrix();
        viewMatrix = glm::inverse(model);
    }

    void CameraComponent::changeProjectionType(ProjectionType projectionType) {
        this->projectionType = projectionType;
    }

    void CameraComponent::onWindowResizeEvent(WindowResizeEvent *event) {
        updateProjectionMatrix();
    }

    glm::mat4 TechEngine::CameraComponent::getViewMatrix() {
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

            ImGui::Text(" Viewport ");
            ImGui::NewLine();
            ImGui::Text("##Width: %d", WindowSettings::width);
            ImGui::NewLine();
            ImGui::Text("##Height: %d", WindowSettings::height);
            ImGui::NewLine();
            ImGui::Text("Aspect ratio: %.2f", WindowSettings::aspectRatio);
            ImGui::NewLine();

            if (ImGui::Button("Update View matrix", ImVec2(300.0f, 300.0f))) {
                std::cout << "Update view matrix" << std::endl;
                updateViewMatrix();
                updateProjectionMatrix();
            }

            ImGui::PopItemWidth();
            ImGui::PopStyleVar();
            ImGui::PopID();
        }

    }

    bool CameraComponent::isMainCamera() {
        return mainCamera;
    }
}
