#include "GameView.hpp"

#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    GameView::GameView(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry) : m_appSystemsRegistry(appSystemsRegistry),
                                                                                                      Panel(editorSystemsRegistry) {
        m_styleVars.push_back({ImGuiStyleVar_WindowPadding, ImVec2(0, 0)});
    }

    void GameView::onInit() {
        frameBufferID = m_appSystemsRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        frameBuffer.unBind();
    }

    void GameView::onUpdate() {
        auto& scene = m_appSystemsRegistry.getSystem<Scene>();
        scene.archetypesManager.runSystem<Camera>([this](Camera& camera) {
            Renderer& renderer = m_appSystemsRegistry.getSystem<Renderer>();
            FrameBuffer& frameBuffer = renderer.getFramebuffer(frameBufferID);
            ImVec2 wsize = ImGui::GetContentRegionAvail();
            frameBuffer.bind();
            frameBuffer.resize(wsize.x, wsize.y);

            camera.updateProjectionMatrix(wsize.x / wsize.y);
            renderer.renderPipeline(camera);
            uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
            ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
            frameBuffer.unBind();
        });
    }
}
