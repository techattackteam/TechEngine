#include "SceneView.hpp"
#include "renderer/RendererSettings.hpp"
#include "scene/SceneHelper.hpp"
#include "components/BoxColliderComponent.hpp"
#include "core/Logger.hpp"

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
        renderer->renderPipeline();
        renderCameraFrustum(currentMainCamera);
        renderColliders();
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
        glm::vec3 color = glm::vec3(1, 1, 1);
        // Render the near plane as a square
        renderer->renderLine(frustumPoints[0], frustumPoints[1], color);
        renderer->renderLine(frustumPoints[1], frustumPoints[3], color);
        renderer->renderLine(frustumPoints[2], frustumPoints[3], color);
        renderer->renderLine(frustumPoints[0], frustumPoints[2], color);

        // Render the far plane as a square
        renderer->renderLine(frustumPoints[4], frustumPoints[5], color);
        renderer->renderLine(frustumPoints[5], frustumPoints[7], color);
        renderer->renderLine(frustumPoints[6], frustumPoints[7], color);
        renderer->renderLine(frustumPoints[4], frustumPoints[6], color);

        // Connect the near and far plane corners to complete the edges
        for (int i = 0; i < 4; ++i) {
            renderer->renderLine(frustumPoints[i], frustumPoints[i + 4], color);
        }

    }

    void SceneView::renderColliders() {
        for (auto &element: Scene::getInstance().getGameObjects()) {
            if (element->isEditorOnly() || !element->hasComponent<BoxColliderComponent>()) {
                continue;
            }
            TransformComponent *transform = element->getComponent<TransformComponent>();
            BoxColliderComponent *collider = element->getComponent<BoxColliderComponent>();

            // Calculate the world space transformation matrix
            glm::mat4 modelMatrix = transform->getModelMatrix();

            // Calculate the half-size of the box based on the scale
            glm::vec3 halfSize = collider->getSize() * 0.5f;

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
            // Render the box collider lines
            // Assuming renderLine(startPosition, endPosition, color) function is available
            // Render the front face
            renderer->renderLine(vertices[0], vertices[1], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[1], vertices[2], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[2], vertices[3], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[3], vertices[0], glm::vec3(1.0f, 0.0f, 0.0f));

            // Render the back face (adjust the Z-coordinate)
            renderer->renderLine(vertices[0], vertices[4], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[1], vertices[5], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[2], vertices[6], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[3], vertices[7], glm::vec3(1.0f, 0.0f, 0.0f));

            // Render the connecting lines between the front and back faces
            renderer->renderLine(vertices[4], vertices[5], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[5], vertices[6], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[6], vertices[7], glm::vec3(1.0f, 0.0f, 0.0f));
            renderer->renderLine(vertices[7], vertices[4], glm::vec3(1.0f, 0.0f, 0.0f));

        }
    }

    void SceneView::changeGuizmoOperation(int operation) {
        guizmo.setOperation(operation);
    }
}
