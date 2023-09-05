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
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void *>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        guizmo.editTransform(ImGui::GetCurrentContext());
        frameBuffer.unBind();
        ImGui::End();
        ImGui::PopStyleVar();
        SceneHelper::changeMainCameraTo(currentMainCamera);
    }

    void SceneView::changeGuizmoOperation(int operation) {
        guizmo.setOperation(operation);
    }
}
