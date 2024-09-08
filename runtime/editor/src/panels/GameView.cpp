#include "GameView.hpp"

#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    GameView::GameView(SystemsRegistry& appRegistry) : Panel(), m_appRegistry(appRegistry) {
        m_styleVars.push_back({ImGuiStyleVar_WindowPadding, ImVec2(0, 0)});
    }

    void GameView::onInit() {
        frameBufferID = m_appRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        FrameBuffer& frameBuffer = m_appRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        frameBuffer.unBind();
    }

    void GameView::onUpdate() {
        Renderer& renderer = m_appRegistry.getSystem<Renderer>();
        if (!m_appRegistry.getSystem<CameraSystem>().hasMainCamera()) {
            return;
        }
        FrameBuffer& frameBuffer = renderer.getFramebuffer(frameBufferID);
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        Camera* camera = nullptr;
        std::vector<Camera> cameras = m_appRegistry.getSystem<Scene>().getComponents<Camera>();
        for (Camera& c: cameras) {
            if (c.isMainCamera()) {
                camera = &c;
                break;
            }
        }
        if (camera == nullptr) {
            TE_LOGGER_WARN("No main camera found.");
            return;
        }
        camera->updateProjectionMatrix(wsize.x / wsize.y);
        renderer.renderPipeline(camera);
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
    }
}
