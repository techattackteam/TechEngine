#include "SceneView.hpp"

#include "components/Components.hpp"

#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    SceneView::SceneView(SystemsRegistry& appSystemsRegistry,
                         const std::vector<Entity>& selectedEntities) : m_appSystemsRegistry(appSystemsRegistry),
                                                                        m_selectedEntities(selectedEntities),
                                                                        guizmo(id, appSystemsRegistry) {
        totalIds++;
        id = totalIds;
    }

    void SceneView::onInit() {
        frameBufferID = m_appSystemsRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        id = totalIds++;
    }

    void SceneView::onUpdate() {
        //sceneCamera->getComponent<CameraComponent>()->update();
        Renderer& renderer = m_appSystemsRegistry.getSystem<Renderer>();
        if (!m_appSystemsRegistry.getSystem<CameraSystem>().hasMainCamera()) {
            return;
        }
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        isWindowHovered = ImGui::IsWindowHovered();
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        Camera* camera = nullptr;
        std::vector<Camera> cameras = m_appSystemsRegistry.getSystem<Scene>().getComponents<Camera>();
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
        //renderColliders();
        m_appSystemsRegistry.getSystem<Renderer>().renderPipeline(camera);
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        guizmo.editTransform(camera, ImGui::GetCurrentContext(), m_selectedEntities);
        frameBuffer.unBind();
    }

    void SceneView::onKeyPressedEvent(Key& key) {
        switch (key.getKeyCode()) {
            case MOUSE_2: {
                mouse2 = true;
                break;
            }
            case MOUSE_3: {
                mouse3 = true;
                break;
            }
        }
        Panel::onKeyPressedEvent(key);
    }

    void SceneView::onKeyReleasedEvent(Key& key) {
        switch (key.getKeyCode()) {
            case MOUSE_2: {
                mouse2 = false;
                moving = false;
                lastUsingId = -1;
                break;
            }
            case MOUSE_3: {
                mouse3 = false;
                moving = false;
                lastUsingId = -1;
                break;
            }
        }
        Panel::onKeyReleasedEvent(key);
    }

    void SceneView::onMouseScrollEvent(float xOffset, float yOffset) {
        if (isWindowHovered && (lastUsingId == -1 || lastUsingId == id)) {
            /*const glm::mat4 inverted = glm::inverse(getSceneCamera()->getComponent<CameraComponent>()->getViewMatrix());
            const glm::vec3 forward = normalize(glm::vec3(inverted[2]));
            if (yOffset == -1.0f) {
                getSceneCamera()->getTransform().translate(forward);
            } else if (yOffset == 1.0f) {
                getSceneCamera()->getTransform().translate(-forward);
            }*/
        }
    }

    void SceneView::onMouseMoveEvent(glm::vec2 delta) {
        if ((lastUsingId == -1 || lastUsingId == id) && (isWindowHovered || moving) && (mouse2 || mouse3)) {
            /*moving = true;
            lastUsingId = id;
            const glm::mat4 inverted = glm::inverse(getSceneCamera()->getComponent<CameraComponent>()->getViewMatrix());
            const glm::vec3 right = normalize(glm::vec3(inverted[0]));
            const glm::vec3 up = normalize(glm::vec3(inverted[1]));
            if (mouse3) {
                const glm::vec3 move = -right * delta.x * 0.01f + up * delta.y * 0.01f;
                getSceneCamera()->getTransform().translate(move);
            }
            if (mouse2) {
                const glm::vec3 rotate = glm::vec3(-delta.y * 0.5f, -delta.x * 0.5f, 0);
                getSceneCamera()->getTransform().rotate(rotate);
            }*/
        }
        //TE_LOGGER_INFO("Mouse scroll event id: {0}, {1}", id, lastUsingId);
    }

    void SceneView::processShortcuts() {
        /*for (Key& key: keysPressed) {
            if (key.getKeyCode() == KeyCode::F) {
                if (selectedGO.size() == 1) {
                    focusOnGameObject(selectedGO.front());
                }
            } else if (key.getKeyCode() == KeyCode::T) {
                changeGuizmoOperation(ImGuizmo::TRANSLATE);
            } else if (key.getKeyCode() == KeyCode::R) {
                changeGuizmoOperation(ImGuizmo::ROTATE);
            } else if (key.getKeyCode() == KeyCode::S) {
                changeGuizmoOperation(ImGuizmo::SCALE);
            } else if (key.getKeyCode() == KeyCode::C) {
                changeGuizmoMode(guizmo.getMode() == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL);
            }
        }*/
    }
}
