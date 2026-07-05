#include "GameView.hpp"

#include "TechEngine/client/events/input/KeyPressedEvent.hpp"
#include "TechEngine/client/events/input/MouseMoveEvent.hpp"
#include "input/Input.hpp"
#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "resources/ResourceSystem.hpp"
#include "scene/SceneManager.hpp"
#include "window/Viewport.hpp"

namespace TechEngine {
    GameView::GameView(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry) : m_appSystemsRegistry(appSystemsRegistry),
                                                                                                      Panel(editorSystemsRegistry) {
        m_styleVars.push_back({ImGuiStyleVar_WindowPadding, ImVec2(0, 0)});
    }

    void GameView::onInit() {
        frameBufferID = m_appSystemsRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        frameBuffer.bind();
        frameBuffer.attachTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0);
        frameBuffer.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);

        frameBuffer.unBind();
    }

    void GameView::onUpdate() {
        auto& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
        Viewport& clientViewport = m_appSystemsRegistry.getSystem<Viewport>();

        ImVec2 panelPos = ImGui::GetWindowPos();
        ImVec2 contentStart = ImGui::GetWindowContentRegionMin();
        clientViewport.position = {panelPos.x + contentStart.x, panelPos.y + contentStart.y};
        clientViewport.size = glm::vec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
        clientViewport.isFocused = ImGui::IsWindowFocused() || ImGui::IsWindowHovered();
        ImVec2 wsize = ImGui::GetContentRegionAvail();

        scene.runSystem<Transform, Camera>([this, wsize](Transform& transform, Camera& camera) {
            camera.updateProjectionMatrix(wsize.x / wsize.y);
            camera.updateViewMatrix(transform.getModelMatrix());
            RenderRequest request;
            request.viewMatrix = camera.getViewMatrix();
            request.projectionMatrix = camera.getProjectionMatrix();
            request.nearPlane = camera.nearPlane;
            request.farPlane = camera.farPlane;
            request.targetFramebufferId = this->frameBufferID;
            request.viewportSize = {wsize.x, wsize.y};
            request.renderMask = Renderer::SCENE_PASS | Renderer::POST_PROCESS_PASS | Renderer::LINE_PASS;
            m_appSystemsRegistry.getSystem<Renderer>().addRequest(request);
        });


        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        uint64_t textureID = frameBuffer.getTextureID(GL_COLOR_ATTACHMENT0);
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
    }

    glm::vec2 GameView::getFrameBufferSize() {
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        return frameBuffer.getSize();
    }

    void GameView::processMouseMoving(glm::vec2 delta) {
        Panel::processMouseMoving(delta);
        ImGuiIO& io = ImGui::GetIO();
        Viewport& clientViewport = m_appSystemsRegistry.getSystem<Viewport>();

        ImVec2 mousePosAbs = io.MousePos;
        double localMouseX = mousePosAbs.x - clientViewport.position.x;
        double localMouseY = mousePosAbs.y - clientViewport.position.y;
        m_appSystemsRegistry.getSystem<Input>().onMouseMove(localMouseX, localMouseY);
    }

    void GameView::processMouseScroll(float yOffset) {
        Panel::processMouseScroll(yOffset);
        m_appSystemsRegistry.getSystem<Input>().onMouseScroll(0.0f, yOffset);
    }

    void GameView::processMouseDragging(glm::vec2 delta, unsigned long long mouseButtons) {
        Panel::processMouseDragging(delta, mouseButtons);
    }

    void GameView::processKeyPressed(ImGuiKey key) {
        m_appSystemsRegistry.getSystem<Input>().onKeyPressed(Key(ImGuiKeyToEngineKeyCode(key)).getKeyCode());
    }

    void GameView::processKeyReleased(ImGuiKey key) {
        m_appSystemsRegistry.getSystem<Input>().onKeyReleased(Key(ImGuiKeyToEngineKeyCode(key)).getKeyCode());
    }

    void GameView::processKeyHold(ImGuiKey key) {
        m_appSystemsRegistry.getSystem<Input>().onKeyHold(Key(ImGuiKeyToEngineKeyCode(key)).getKeyCode());
    }
}
