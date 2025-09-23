#include "GameView.hpp"

#include "events/input/KeyPressedEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "input/Input.hpp"
#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "scene/ScenesManager.hpp"
#include "window/Viewport.hpp"

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
        auto& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
        Viewport& clientViewport = m_appSystemsRegistry.getSystem<Viewport>();

        ImVec2 panelPos = ImGui::GetWindowPos();
        ImVec2 contentStart = ImGui::GetWindowContentRegionMin();
        clientViewport.position = {panelPos.x + contentStart.x, panelPos.y + contentStart.y};
        clientViewport.size = glm::vec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
        clientViewport.isFocused = ImGui::IsWindowFocused() || ImGui::IsWindowHovered();

        //scene.runSystem<Transform, Camera>([this](Transform& transform, Camera& camera) {
        //    FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        //    ImVec2 wsize = ImGui::GetContentRegionAvail();
        //    frameBuffer.bind();
        //    frameBuffer.resize(wsize.x, wsize.y);
        //    camera.updateProjectionMatrix(wsize.x / wsize.y);
        //    camera.updateViewMatrix(transform.getModelMatrix());
//
        //    RenderRequest request;
        //    request.viewMatrix = camera.getViewMatrix(); // This makes a copy
        //    request.projectionMatrix = camera.getProjectionMatrix(); // This makes a copy
        //    request.targetFramebufferId = this->frameBufferID;
        //    request.viewportSize = {wsize.x, wsize.y};
        //    request.renderMask = Renderer::GEOMETRY_PASS | Renderer::LINE_PASS;
//
        //    // 3. Submit the request to the renderer.
        //    m_appSystemsRegistry.getSystem<Renderer>().addRequest(request);
        //});

        ImVec2 wsize = ImGui::GetContentRegionAvail();
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
        //glm::vec2 mousePos = m_appSystemsRegistry.getSystem<Input>().getMouse().getPosition();
        //TE_LOGGER_INFO("Panel: {0}, Hovered: {1}, Focused: {2}, Mouse: ({3},{4})", m_name, m_isPanelHovered, m_isPanelFocused, mousePos.x, mousePos.y);
    }

    glm::vec2 GameView::getFrameBufferSize() {
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        return glm::vec2(frameBuffer.width, frameBuffer.height);
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

    void GameView::processKeyPressed(Key key) {
        m_appSystemsRegistry.getSystem<Input>().onKeyPressed(key.getKeyCode());
    }

    void GameView::processKeyReleased(Key key) {
        m_appSystemsRegistry.getSystem<Input>().onKeyReleased(key.getKeyCode());
    }

    void GameView::processKeyHold(Key key) {
        m_appSystemsRegistry.getSystem<Input>().onKeyHold(key.getKeyCode());
    }
}
