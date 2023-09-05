#include "SceneView.hpp"
#include "renderer/RendererSettings.hpp"
#include "scene/SceneHelper.hpp"

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

    void SceneView::changeGuizmoOperation(int operation) {
        guizmo.setOperation(operation);
    }
}
