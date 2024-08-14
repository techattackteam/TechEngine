#include "SceneView.hpp"

#include "components/physics/BoxColliderComponent.hpp"
#include "core/Logger.hpp"
#include "PanelsManager.hpp"
#include "components/physics/SphereCollider.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "components/physics/RigidBody.hpp"
#include "physics/PhysicsDebugRenderer.hpp"

namespace TechEngine {
    SceneView::SceneView(const std::string& name,
                         SystemsRegistry& editorRegistry,
                         SystemsRegistry& appRegistry,
                         std::vector<GameObject*>& selectedGO)
        : appRegistry(appRegistry),
          guizmo(id),
          selectedGO(selectedGO),
          Panel(name, editorRegistry) {
    }

    SceneView::~SceneView() {
        delete sceneCamera;
    }

    void SceneView::init() {
        Panel::init();
        frameBufferID = appRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        sceneCamera = new GameObject("SceneCamera", appRegistry);
        sceneCamera->addComponent<CameraComponent>();
        id = totalIds++;
    }

    void SceneView::onUpdate() {
        if (ImGui::IsWindowCollapsed()) {
            return;
        }
        sceneCamera->getComponent<CameraComponent>()->update();
        FrameBuffer& frameBuffer = appRegistry.getSystem<Renderer>().getFramebuffer(frameBufferID);
        isWindowHovered = ImGui::IsWindowHovered();
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        sceneCamera->getComponent<CameraComponent>()->updateProjectionMatrix(wsize.x / wsize.y);
        if (appRegistry.getSystem<SceneManager>().getScene().hasMainCamera()) {
            renderCameraFrustum(appRegistry.getSystem<SceneManager>().getScene().getMainCamera());
        }
        renderColliders();
        appRegistry.getSystem<Renderer>().renderPipeline(appRegistry.getSystem<SceneManager>().getScene(), sceneCamera->getComponent<CameraComponent>());
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        guizmo.editTransform(sceneCamera->getComponent<CameraComponent>(), ImGui::GetCurrentContext(), selectedGO);
        frameBuffer.unBind();
    }

    void SceneView::renderCameraFrustum(CameraComponent* camera) {
        std::vector<glm::vec3> frustumPoints;

        // Define the 8 corners of the frustum in NDC
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

        // Inverse of the combined view-projection matrix
        glm::mat4 invViewProjection = glm::inverse(camera->getProjectionMatrix() * camera->getViewMatrix());

        // Transform NDC points to world space
        for (const glm::vec4& ndcPoint: ndcPoints) {
            glm::vec4 worldPoint = invViewProjection * ndcPoint;
            worldPoint /= worldPoint.w; // Homogeneous divide
            frustumPoints.push_back(glm::vec3(worldPoint));
        }
        glm::vec4 color;
        if (selectedGO.size() == 1
            && camera->getGameObject() == selectedGO.front()) {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        } else {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);
        }
        // Render the near plane as a square
        appRegistry.getSystem<Renderer>().createLine(frustumPoints[0], frustumPoints[1], color);
        appRegistry.getSystem<Renderer>().createLine(frustumPoints[1], frustumPoints[3], color);
        appRegistry.getSystem<Renderer>().createLine(frustumPoints[2], frustumPoints[3], color);
        appRegistry.getSystem<Renderer>().createLine(frustumPoints[0], frustumPoints[2], color);

        // Render the far plane as a square
        appRegistry.getSystem<Renderer>().createLine(frustumPoints[4], frustumPoints[5], color);
        appRegistry.getSystem<Renderer>().createLine(frustumPoints[5], frustumPoints[7], color);
        appRegistry.getSystem<Renderer>().createLine(frustumPoints[6], frustumPoints[7], color);
        appRegistry.getSystem<Renderer>().createLine(frustumPoints[4], frustumPoints[6], color);

        // Connect the near and far plane corners to complete the edges
        for (int i = 0; i < 4; ++i) {
            appRegistry.getSystem<Renderer>().createLine(frustumPoints[i], frustumPoints[i + 4], color);
        }
    }

    void SceneView::renderColliders() {
        /*for (auto& element: appRegistry.getSystem<SceneManager>().getScene().getGameObjects()) {
            if (element->isEditorOnly()) {
                continue;
            }
            if (element->hasComponent<BoxColliderComponent>()) {
                renderBoxCollider(element);
            }
            if (element->hasComponent<SphereCollider>()) {
                renderSphereCollider(element);
            }
            if (element->hasComponent<CylinderCollider>()) {
                renderCylinderCollier(element);
            }
        }*/
        //appRegistry.getSystem<PhysicsEngine>().drawDebug(appRegistry.getSystem<PhysicsDebugRenderer>());
    }

    void SceneView::focusOnGameObject(GameObject* gameObject) {
        glm::vec3 position = gameObject->getComponent<TransformComponent>()->position;
        glm::vec3 forward = glm::normalize(glm::vec3(glm::inverse(getSceneCamera()->getComponent<CameraComponent>()->getViewMatrix())[2]));
        getSceneCamera()->getTransform().position = position + forward * 2.5f;
    }

    void SceneView::renderBoxCollider(GameObject* gameObject) {
        TransformComponent* transform = gameObject->getComponent<TransformComponent>();
        BoxColliderComponent* collider = gameObject->getComponent<BoxColliderComponent>();

        // Calculate the world space transformation matrix
        glm::mat4 modelMatrix = transform->getModelMatrix();

        float offset = 0.005f;
        // Calculate the half-size of the box based on the scale
        glm::vec3 halfSize = collider->getSize() / 2.0f;

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
            vertices[i].x = vertices[i].x >= 0 ? vertices[i].x + offset : vertices[i].x - offset;
            vertices[i].y = vertices[i].y >= 0 ? vertices[i].y + offset : vertices[i].y - offset;
            vertices[i].z = vertices[i].z >= 0 ? vertices[i].z + offset : vertices[i].z - offset;
        }
        glm::vec4 color = getColor(gameObject);
        appRegistry.getSystem<Renderer>().createLine(vertices[0], vertices[1], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[1], vertices[2], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[2], vertices[3], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[3], vertices[0], color);

        // Render the back face (adjust the Z-coordinate)
        appRegistry.getSystem<Renderer>().createLine(vertices[0], vertices[4], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[1], vertices[5], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[2], vertices[6], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[3], vertices[7], color);

        // Render the connecting lines between the front and back faces
        appRegistry.getSystem<Renderer>().createLine(vertices[4], vertices[5], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[5], vertices[6], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[6], vertices[7], color);
        appRegistry.getSystem<Renderer>().createLine(vertices[7], vertices[4], color);
    }

    void SceneView::renderSphereCollider(GameObject* gameObject) {
        TransformComponent* transform = gameObject->getComponent<TransformComponent>();
        SphereCollider* collider = gameObject->getComponent<SphereCollider>();
        const int numSegments = 128;
        const float segmentAngle = glm::two_pi<float>() / static_cast<float>(numSegments);
        const glm::vec3 center = transform->position + collider->getOffset();
        const float offset = 0.005f;
        const float radius = collider->getRadius() + offset;
        glm::vec4 color = getColor(gameObject);

        // Create a circle along the X-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = glm::vec3(radius, radius, 0.0f) * glm::vec3(glm::cos(theta1), glm::sin(theta1), 0.0f);
            glm::vec3 point2 = glm::vec3(radius, radius, 0.0f) * glm::vec3(glm::cos(theta2), glm::sin(theta2), 0.0f);
            point1 = glm::vec3(transform->getModelMatrix() * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(transform->getModelMatrix() * glm::vec4(point2, 1.0f));
            // Create a line segment along the X-axis with the specified color
            appRegistry.getSystem<Renderer>().createLine(point1, point2, color);
        }

        // Create a circle along the Y-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = glm::vec3(0.0f, radius, radius) * glm::vec3(1.0f, glm::cos(theta1), glm::sin(theta1));
            glm::vec3 point2 = glm::vec3(0.0f, radius, radius) * glm::vec3(1.0f, glm::cos(theta2), glm::sin(theta2));
            point1 = glm::vec3(transform->getModelMatrix() * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(transform->getModelMatrix() * glm::vec4(point2, 1.0f));

            // Create a line segment along the Y-axis with the specified color
            appRegistry.getSystem<Renderer>().createLine(point1, point2, color);
        }

        // Create a circle along the Z-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = glm::vec3(radius, 0.0f, radius) * glm::vec3(glm::cos(theta1), 1.0f, glm::sin(theta1));
            glm::vec3 point2 = glm::vec3(radius, 0.0f, radius) * glm::vec3(glm::cos(theta2), 1.0f, glm::sin(theta2));
            point1 = glm::vec3(transform->getModelMatrix() * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(transform->getModelMatrix() * glm::vec4(point2, 1.0f));

            // Create a line segment along the Z-axis with the specified color
            appRegistry.getSystem<Renderer>().createLine(point1, point2, color);
        }
    }

    void SceneView::renderCylinderCollier(GameObject* gameObject) {
        TransformComponent* transform = gameObject->getComponent<TransformComponent>();
        CylinderCollider* collider = gameObject->getComponent<CylinderCollider>();
        glm::vec3 position = transform->position;
        glm::quat orientation = transform->getOrientation();
        const float offset = 0.005f;
        float radius = collider->getRadius() + offset;
        float height = collider->getHeight() + offset;
        // Calculate the vertices to outline the cylinder
        const int numSegments = 32;
        std::vector<glm::vec3> vertices;
        glm::vec4 color = getColor(gameObject);
        for (int i = 0; i <= numSegments; ++i) {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(numSegments);
            float x = radius * glm::cos(theta);
            float z = radius * glm::sin(theta);

            // Calculate top and bottom points
            glm::vec3 topPoint = glm::vec3(x, height / 2.0f, z);
            glm::vec3 bottomPoint = glm::vec3(x, -height / 2.0f, z);

            // Apply rotation and translation
            topPoint = glm::vec3(transform->getModelMatrix() * glm::vec4(topPoint, 1.0f));
            bottomPoint = glm::vec3(transform->getModelMatrix() * glm::vec4(bottomPoint, 1.0f));

            // Add the points to the vertices array
            vertices.push_back(topPoint);
            vertices.push_back(bottomPoint);

            // Draw lines to connect the points
            if (i > 0) {
                glm::vec3 prevTopPoint = vertices[(i - 1) * 2];
                glm::vec3 prevBottomPoint = vertices[(i - 1) * 2 + 1];

                // Render lines for the sides of the cylinder
                appRegistry.getSystem<Renderer>().createLine(prevTopPoint, topPoint, color); // Red lines
                appRegistry.getSystem<Renderer>().createLine(prevBottomPoint, bottomPoint, color); // Red lines

                // Render lines to connect the top and bottom points
                appRegistry.getSystem<Renderer>().createLine(prevTopPoint, prevBottomPoint, color); // Red lines
            }
        }

        // Connect the first and last points to close the cylinder
        glm::vec3 firstTopPoint = vertices[0];
        glm::vec3 firstBottomPoint = vertices[1];
        glm::vec3 lastTopPoint = vertices[vertices.size() - 2];
        glm::vec3 lastBottomPoint = vertices[vertices.size() - 1];

        // Render lines for the sides of the cylinder
        appRegistry.getSystem<Renderer>().createLine(lastTopPoint, firstTopPoint, color); // Red lines
        appRegistry.getSystem<Renderer>().createLine(lastBottomPoint, firstBottomPoint, color); // Red lines

        // Render lines to connect the top and bottom points
        appRegistry.getSystem<Renderer>().createLine(lastTopPoint, lastBottomPoint, color); // Red lines
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
            const glm::mat4 inverted = glm::inverse(getSceneCamera()->getComponent<CameraComponent>()->getViewMatrix());
            const glm::vec3 forward = normalize(glm::vec3(inverted[2]));
            if (yOffset == -1.0f) {
                getSceneCamera()->getTransform().translate(forward);
            } else if (yOffset == 1.0f) {
                getSceneCamera()->getTransform().translate(-forward);
            }
        }
    }

    void SceneView::onMouseMoveEvent(glm::vec2 delta) {
        if ((lastUsingId == -1 || lastUsingId == id) && (isWindowHovered || moving) && (mouse2 || mouse3)) {
            moving = true;
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
            }
        }
        //TE_LOGGER_INFO("Mouse scroll event id: {0}, {1}", id, lastUsingId);
    }

    void SceneView::processShortcuts() {
        for (Key& key: keysPressed) {
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
        }
    }

    glm::vec4 SceneView::getColor(GameObject* gameObject) {
        if (std::find(selectedGO.begin(), selectedGO.end(), gameObject) != selectedGO.end()) {
            if (gameObject->hasComponent<RigidBody>()) {
                return glm::vec4(0, 1.0f, 0, 1.0f);
            } else {
                return glm::vec4(1.0f, 0, 0, 1.0f);
            }
        } else {
            if (gameObject->hasComponent<RigidBody>()) {
                return glm::vec4(0, 1.0f, 0, 0.3f);
            } else {
                return glm::vec4(1.0f, 0, 0, 0.3f);
            }
        }
    }

    void SceneView::changeGuizmoOperation(int operation) {
        guizmo.setOperation(operation);
    }

    void SceneView::changeGuizmoMode(int mode) {
        guizmo.setMode(mode);
    }

    int SceneView::getGuizmoMode() const {
        return guizmo.getMode();
    }
}
