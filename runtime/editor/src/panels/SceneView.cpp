#include "SceneView.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>

#include "components/Components.hpp"
#include "components/ComponentsFactory.hpp"
#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "scene/ScenesManager.hpp"
#include "scene/TransformSystem.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    SceneView::SceneView(SystemsRegistry& editorSystemsRegistry,
                         SystemsRegistry& appSystemsRegistry,
                         const std::vector<Entity>& selectedEntities) : m_appSystemsRegistry(appSystemsRegistry),
                                                                        m_selectedEntities(selectedEntities),
                                                                        guizmo(id, appSystemsRegistry),
                                                                        cameraTransform(ComponentsFactory::createTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f))),
                                                                        /*sceneCamera(ComponentsFactory::createCamera(45.0f, 0.1f, 1000.0f, 1080.0f / 720.0f)),*/
                                                                        Panel(editorSystemsRegistry) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        totalIds++;
        id = totalIds;
    }

    void SceneView::onInit() {
        frameBufferID = m_appSystemsRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        id = totalIds++;
    }

    void SceneView::onUpdate() {
        Renderer& renderer = m_appSystemsRegistry.getSystem<Renderer>();
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        isWindowHovered = ImGui::IsWindowHovered();
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        sceneCamera.updateProjectionMatrix(wsize.x / wsize.y);
        sceneCamera.updateViewMatrix(m_appSystemsRegistry.getSystem<TransformSystem>().getModelMatrix(cameraTransform));

        m_appSystemsRegistry.getSystem<PhysicsEngine>().updateBodies();
        m_appSystemsRegistry.getSystem<PhysicsEngine>().renderBodies();
        renderCameraFrustum();
        renderer.renderPipeline(sceneCamera);
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        guizmo.editTransform(&sceneCamera, ImGui::GetCurrentContext(), m_selectedEntities);
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
            const glm::mat4 inverted = glm::inverse(sceneCamera.getViewMatrix());
            const glm::vec3 forward = normalize(glm::vec3(inverted[2]));
            if (yOffset == -1.0f) {
                m_appSystemsRegistry.getSystem<TransformSystem>().translate(cameraTransform, forward);
            } else if (yOffset == 1.0f) {
                m_appSystemsRegistry.getSystem<TransformSystem>().translate(cameraTransform, -forward);
            }
        }
    }

    void SceneView::onMouseMoveEvent(glm::vec2 delta) {
        if ((lastUsingId == -1 || lastUsingId == id) && (isWindowHovered || moving) && (mouse2 || mouse3)) {
            moving = true;
            lastUsingId = id;
            const glm::mat4 inverted = glm::inverse(sceneCamera.getViewMatrix());
            const glm::vec3 right = normalize(glm::vec3(inverted[0]));
            const glm::vec3 up = normalize(glm::vec3(inverted[1]));
            if (mouse3) {
                const glm::vec3 move = -right * delta.x * 0.01f + up * delta.y * 0.01f;
                m_appSystemsRegistry.getSystem<TransformSystem>().translate(cameraTransform, move);
            }
            if (mouse2) {
                const glm::vec3 rotate = glm::vec3(-delta.y * 0.5f, -delta.x * 0.5f, 0);
                m_appSystemsRegistry.getSystem<TransformSystem>().rotate(cameraTransform, rotate);
            }
        }
    }

    void SceneView::changeGuizmoOperation(ImGuizmo::OPERATION operation) {
        guizmo.setOperation(operation);
    }

    void SceneView::changeGuizmoMode(ImGuizmo::MODE mode) {
        guizmo.setMode(mode);
    }

    void SceneView::processShortcuts() {
        for (Key& key: m_keysPressed) {
            if (key.getKeyCode() == KeyCode::F) {
                if (m_selectedEntities.size() == 1) {
                    //focusOnGameObject(selectedGO.front());
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
        }
    }

    void SceneView::renderCameraFrustum() {
        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
        // Define the 8 corners of the frustum in NDC
        scene.runSystem<Camera, Transform>([this](Camera& camera, Transform& transform) {
            std::vector<glm::vec3> frustumPoints;
            std::vector<glm::vec4> ndcPoints = {
                glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f), // Near bottom left
                glm::vec4(1.0f, -1.0f, -1.0f, 1.0f), // Near bottom right
                glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f), // Near top left
                glm::vec4(1.0f, 1.0f, -1.0f, 1.0f), // Near top right
                glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f), // Far bottom left
                glm::vec4(1.0f, -1.0f, 1.0f, 1.0f), // Far bottom right
                glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f), // Far top left
                glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) // Far top right
            };
            TransformSystem& transformSystem = m_appSystemsRegistry.getSystem<TransformSystem>();

            // Inverse of the combined view-projection matrix
            glm::mat4 invViewProjection = glm::inverse(camera.getProjectionMatrix() * glm::inverse(transformSystem.getModelMatrix(transform)));

            // Transform NDC points to world space
            for (const glm::vec4& ndcPoint: ndcPoints) {
                glm::vec4 worldPoint = invViewProjection * ndcPoint;
                worldPoint /= worldPoint.w; // Homogeneous divide
                frustumPoints.emplace_back(worldPoint);
            }
            glm::vec4 color;
            color = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);
            // Render the near plane as a square
            camera.updateProjectionMatrix(camera.getAspectRatio());
            m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[0], frustumPoints[1], color);
            m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[1], frustumPoints[3], color);
            m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[2], frustumPoints[3], color);
            m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[0], frustumPoints[2], color);

            // Render the far plane as a square
            m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[4], frustumPoints[5], color);
            m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[5], frustumPoints[7], color);
            m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[6], frustumPoints[7], color);
            m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[4], frustumPoints[6], color);

            // Connect the near and far plane corners to complete the edges
            for (int i = 0; i < 4; ++i) {
                m_appSystemsRegistry.getSystem<Renderer>().createLine(frustumPoints[i], frustumPoints[i + 4], color);
            }
        });
    }
}
