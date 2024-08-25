#include "GameView.hpp"

#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    GameView::GameView(SystemsRegistry& appRegistry) : Panel(), m_appRegistry(appRegistry) {
    }

    void GameView::onInit() {
        frameBufferID = m_appRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        FrameBuffer& frameBuffer = m_appRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        frameBuffer.unBind();
    }

    void GameView::onUpdate() {
        Renderer& renderer = m_appRegistry.getSystem<Renderer>();
        if (!m_appRegistry.getSystem<Scene>().hasMainCamera()) {
            return;
        }
        FrameBuffer& frameBuffer = renderer.getFramebuffer(frameBufferID);
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        m_appRegistry.getSystem<Scene>().getMainCamera().updateProjectionMatrix(wsize.x / wsize.y);
        renderer.renderPipeline(m_appRegistry.getSystem<Scene>());
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
    }
}
