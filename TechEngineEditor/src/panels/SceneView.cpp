#include "SceneView.hpp"
#include "renderer/RendererSettings.hpp"
#include "scene/SceneHelper.hpp"
#include "components/physics/BoxColliderComponent.hpp"
#include "core/Logger.hpp"
#include "PanelsManager.hpp"
#include "components/physics/SphereCollider.hpp"

namespace TechEngine {
    SceneView::SceneView(Renderer &renderer) : renderer(&renderer), Panel("Scene") {
        frameBufferID = renderer.createFramebuffer(RendererSettings::width, RendererSettings::height);
        sceneCamera = new SceneCamera();
    }


    void SceneView::onUpdate() {
        CameraComponent *currentMainCamera = SceneHelper::mainCamera;
        if (!currentMainCamera) {
            return;
        }
        sceneCamera->getComponent<CameraComponent>()->update();
        SceneHelper::changeMainCameraTo(sceneCamera->getComponent<CameraComponent>());
        FrameBuffer &frameBuffer = renderer->getFramebuffer(frameBufferID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin(name.c_str());
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        renderCameraFrustum(currentMainCamera);
        renderColliders();
        renderer->renderPipeline();
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void *>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        guizmo.editTransform(ImGui::GetCurrentContext());
        frameBuffer.unBind();
        ImGui::End();
        ImGui::PopStyleVar();
        SceneHelper::changeMainCameraTo(currentMainCamera);
    }

    void SceneView::renderCameraFrustum(CameraComponent *camera) {
        std::vector<glm::vec3> frustumPoints;

        // Define the 8 corners of the frustum in NDC
        std::vector<glm::vec4> ndcPoints = {
                glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), // Near bottom left
                glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),  // Near bottom right
                glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f),  // Near top left
                glm::vec4(1.0f, 1.0f, -1.0f, 1.0f),   // Near top right
                glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),  // Far bottom left
                glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),   // Far bottom right
                glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f),   // Far top left
                glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)     // Far top right
        };

        // Inverse of the combined view-projection matrix
        glm::mat4 invViewProjection = glm::inverse(camera->getProjectionMatrix() * camera->getViewMatrix());

        // Transform NDC points to world space
        for (const glm::vec4 &ndcPoint: ndcPoints) {
            glm::vec4 worldPoint = invViewProjection * ndcPoint;
            worldPoint /= worldPoint.w; // Homogeneous divide
            frustumPoints.push_back(glm::vec3(worldPoint));
        }
        glm::vec4 color;
        if (camera->getGameObject() == PanelsManager::getInstance().getSelectedGameObject()) {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);
        }
        // Render the near plane as a square
        renderer->createLine(frustumPoints[0], frustumPoints[1], color);
        renderer->createLine(frustumPoints[1], frustumPoints[3], color);
        renderer->createLine(frustumPoints[2], frustumPoints[3], color);
        renderer->createLine(frustumPoints[0], frustumPoints[2], color);

        // Render the far plane as a square
        renderer->createLine(frustumPoints[4], frustumPoints[5], color);
        renderer->createLine(frustumPoints[5], frustumPoints[7], color);
        renderer->createLine(frustumPoints[6], frustumPoints[7], color);
        renderer->createLine(frustumPoints[4], frustumPoints[6], color);

        // Connect the near and far plane corners to complete the edges
        for (int i = 0; i < 4; ++i) {
            renderer->createLine(frustumPoints[i], frustumPoints[i + 4], color);
        }

    }

    void SceneView::renderColliders() {
        for (auto &element: Scene::getInstance().getAllGameObjects()) {
            if (element->isEditorOnly()) {
                continue;
            }
            if (element->hasComponent<BoxColliderComponent>()) {
                renderBoxCollider(element);
            }
            if (element->hasComponent<SphereCollider>()) {
                renderSphereCollider(element);
            }
        }
    }

    void SceneView::renderBoxCollider(GameObject *gameObject) {
        TransformComponent *transform = gameObject->getComponent<TransformComponent>();
        BoxColliderComponent *collider = gameObject->getComponent<BoxColliderComponent>();

        // Calculate the world space transformation matrix
        glm::mat4 modelMatrix = transform->getModelMatrix();

        float offset = 0.005f;
        // Calculate the half-size of the box based on the scale
        glm::vec3 halfSize = collider->getSize() * 0.5f + offset;


        // Define the vertices of the box in local space (unrotated)
        glm::vec3 vertices[8] = {
                glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                glm::vec3(halfSize.x, -halfSize.y, -halfSize.z),
                glm::vec3(halfSize.x, halfSize.y, -halfSize.z),
                glm::vec3(-halfSize.x, halfSize.y, -halfSize.z),
                glm::vec3(-halfSize.x, -halfSize.y, halfSize.z),
                glm::vec3(halfSize.x, -halfSize.y, halfSize.z),
                glm::vec3(halfSize.x, halfSize.y, halfSize.z),
                glm::vec3(-halfSize.x, halfSize.y, halfSize.z)
        };

        // Transform the vertices from local space to world space
        for (int i = 0; i < 8; i++) {
            vertices[i] = glm::vec3(modelMatrix * glm::vec4(vertices[i], 1.0f));
        }
        glm::vec4 color;
        if (gameObject == PanelsManager::getInstance().getSelectedGameObject()) {
            color = glm::vec4(1.0f, 0, 0, 1.0f);
        } else {
            color = glm::vec4(1.0f, 0, 0, 0.3f);
        }
        renderer->createLine(vertices[0], vertices[1], color);
        renderer->createLine(vertices[1], vertices[2], color);
        renderer->createLine(vertices[2], vertices[3], color);
        renderer->createLine(vertices[3], vertices[0], color);

        // Render the back face (adjust the Z-coordinate)
        renderer->createLine(vertices[0], vertices[4], color);
        renderer->createLine(vertices[1], vertices[5], color);
        renderer->createLine(vertices[2], vertices[6], color);
        renderer->createLine(vertices[3], vertices[7], color);

        // Render the connecting lines between the front and back faces
        renderer->createLine(vertices[4], vertices[5], color);
        renderer->createLine(vertices[5], vertices[6], color);
        renderer->createLine(vertices[6], vertices[7], color);
        renderer->createLine(vertices[7], vertices[4], color);
    }

    void SceneView::renderSphereCollider(GameObject *gameObject) {
        TransformComponent *transform = gameObject->getComponent<TransformComponent>();
        SphereCollider *collider = gameObject->getComponent<SphereCollider>();
        const int numSegments = 128;
        const float segmentAngle = glm::two_pi<float>() / static_cast<float>(numSegments);
        const glm::vec3 center = transform->position + collider->getOffset();
        const float offset = 0.005f;
        const float radius = collider->getRadius() / 2 + offset;
        glm::vec4 color;
        if (gameObject == PanelsManager::getInstance().getSelectedGameObject()) {
            color = glm::vec4(1.0f, 0, 0, 1.0f);
        } else {
            color = glm::vec4(1.0f, 0, 0, 0.3f);
        }

        // Create a circle along the X-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = center + glm::vec3(radius, radius, 0.0f) * glm::vec3(glm::cos(theta1), glm::sin(theta1), 0.0f);
            glm::vec3 point2 = center + glm::vec3(radius, radius, 0.0f) * glm::vec3(glm::cos(theta2), glm::sin(theta2), 0.0f);

            // Create a line segment along the X-axis with the specified color
            renderer->createLine(point1, point2, color);
        }

        // Create a circle along the Y-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = center + glm::vec3(0.0f, radius, radius) * glm::vec3(1.0f, glm::cos(theta1), glm::sin(theta1));
            glm::vec3 point2 = center + glm::vec3(0.0f, radius, radius) * glm::vec3(1.0f, glm::cos(theta2), glm::sin(theta2));

            // Create a line segment along the Y-axis with the specified color
            renderer->createLine(point1, point2, color);
        }

        // Create a circle along the Z-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = center + glm::vec3(radius, 0.0f, radius) * glm::vec3(glm::cos(theta1), 1.0f, glm::sin(theta1));
            glm::vec3 point2 = center + glm::vec3(radius, 0.0f, radius) * glm::vec3(glm::cos(theta2), 1.0f, glm::sin(theta2));

            // Create a line segment along the Z-axis with the specified color
            renderer->createLine(point1, point2, color);
        }
    }

    void SceneView::changeGuizmoOperation(int operation) {
        guizmo.setOperation(operation);
    }
}