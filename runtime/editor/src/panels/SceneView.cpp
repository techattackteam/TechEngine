#include "SceneView.hpp"

#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
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

        m_appSystemsRegistry.getSystem<PhysicsEngine>().renderBodies();
        renderCameraFrustum();
        renderColliders();
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

    void SceneView::renderColliders() {
        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Tag, Transform, BoxCollider>([this](Tag& tag, Transform& transform, BoxCollider& collider) {
            glm::vec4 color = getColor(tag, true);
            renderBox(transform, collider.center, collider.size, color);
        });

        scene.runSystem<Tag, Transform, SphereCollider>([this](Tag& tag, Transform& transform, SphereCollider& collider) {
            glm::vec4 color = getColor(tag, true);
            renderSphere(transform, collider.center, collider.radius, color);
        });

        scene.runSystem<Tag, Transform, CapsuleCollider>([this](Tag& tag, Transform& transform, CapsuleCollider& collider) {
            glm::vec4 color = getColor(tag, true);
            renderCapsule(transform, collider.center, collider.radius, collider.height, color);
        });

        scene.runSystem<Tag, Transform, CylinderCollider>([this](Tag& tag, Transform& transform, CylinderCollider& collider) {
            glm::vec4 color = getColor(tag, true);
            renderCylinder(transform, collider.center, collider.radius, collider.height, color);
        });


        scene.runSystem<Tag, Transform, BoxTrigger>([this](Tag& tag, Transform& transform, BoxTrigger& trigger) {
            glm::vec4 color = getColor(tag, false);
            renderBox(transform, trigger.center, trigger.size, color);
        });

        scene.runSystem<Tag, Transform, SphereTrigger>([this](Tag& tag, Transform& transform, SphereTrigger& trigger) {
            glm::vec4 color = getColor(tag, false);
            renderSphere(transform, trigger.center, trigger.radius, color);
        });

        scene.runSystem<Tag, Transform, CapsuleTrigger>([this](Tag& tag, Transform& transform, CapsuleTrigger& trigger) {
            glm::vec4 color = getColor(tag, false);
            renderCapsule(transform, trigger.center, trigger.radius, trigger.height, color);
        });

        scene.runSystem<Tag, Transform, CylinderTrigger>([this](Tag& tag, Transform& transform, CylinderTrigger& trigger) {
            glm::vec4 color = getColor(tag, false);
            renderCylinder(transform, trigger.center, trigger.radius, trigger.height, color);
        });
    }

    void SceneView::renderBox(Transform& transform, glm::vec3 center, glm::vec3 scale, glm::vec4 color) const {
        // Calculate the world space transformation matrix
        TransformSystem& transformSystem = m_appSystemsRegistry.getSystem<TransformSystem>();
        Renderer& renderer = m_appSystemsRegistry.getSystem<Renderer>();
        Transform tempTransform = transform;
        tempTransform.position += center;
        glm::mat4 modelMatrix = transformSystem.getModelMatrix(tempTransform);

        float offset = 0.05f;
        // Calculate the half-size of the box based on the scale
        glm::vec3 halfSize = glm::vec3(scale * 0.5f);

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
            glm::vec3 offsetVec;
            offsetVec.x = vertices[i].x >= 0 ? offset : -offset;
            offsetVec.y = vertices[i].y >= 0 ? offset : -offset;
            offsetVec.z = vertices[i].z >= 0 ? offset : -offset;
            vertices[i] = glm::vec3(modelMatrix * glm::vec4(vertices[i], 1.0f));
            vertices[i] += offsetVec;
        }
        renderer.createLine(vertices[0], vertices[1], color);
        renderer.createLine(vertices[1], vertices[2], color);
        renderer.createLine(vertices[2], vertices[3], color);
        renderer.createLine(vertices[3], vertices[0], color);

        // Render the back face (adjust the Z-coordinate)
        renderer.createLine(vertices[0], vertices[4], color);
        renderer.createLine(vertices[1], vertices[5], color);
        renderer.createLine(vertices[2], vertices[6], color);
        renderer.createLine(vertices[3], vertices[7], color);

        // Render the connecting lines between the front and back faces
        renderer.createLine(vertices[4], vertices[5], color);
        renderer.createLine(vertices[5], vertices[6], color);
        renderer.createLine(vertices[6], vertices[7], color);
        renderer.createLine(vertices[7], vertices[4], color);
    }

    void SceneView::renderSphere(Transform& transform, glm::vec3 center, float radius, glm::vec4 color) const {
        // Calculate the world space transformation matrix
        TransformSystem& transformSystem = m_appSystemsRegistry.getSystem<TransformSystem>();
        Transform tempTransform = transform;
        tempTransform.position += center;
        tempTransform.scale = glm::vec3(std::max(tempTransform.scale.x, std::max(tempTransform.scale.y, tempTransform.scale.z)));
        glm::mat4 modelMatrix = transformSystem.getModelMatrix(tempTransform);

        const int numSegments = 128;
        const float segmentAngle = glm::two_pi<float>() / static_cast<float>(numSegments);
        const float offset = 0.005f;
        radius = radius + offset;

        // Create a circle along the X-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = glm::vec3(radius, radius, 0.0f) * glm::vec3(glm::cos(theta1), glm::sin(theta1), 0.0f);
            glm::vec3 point2 = glm::vec3(radius, radius, 0.0f) * glm::vec3(glm::cos(theta2), glm::sin(theta2), 0.0f);
            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));
            // Create a line segment along the X-axis with the specified color
            m_appSystemsRegistry.getSystem<Renderer>().createLine(point1, point2, color);
        }

        // Create a circle along the Y-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = glm::vec3(0.0f, radius, radius) * glm::vec3(1.0f, glm::cos(theta1), glm::sin(theta1));
            glm::vec3 point2 = glm::vec3(0.0f, radius, radius) * glm::vec3(1.0f, glm::cos(theta2), glm::sin(theta2));
            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));

            // Create a line segment along the Y-axis with the specified color
            m_appSystemsRegistry.getSystem<Renderer>().createLine(point1, point2, color);
        }

        // Create a circle along the Z-axis
        for (int i = 0; i < numSegments; ++i) {
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = glm::vec3(radius, 0.0f, radius) * glm::vec3(glm::cos(theta1), 1.0f, glm::sin(theta1));
            glm::vec3 point2 = glm::vec3(radius, 0.0f, radius) * glm::vec3(glm::cos(theta2), 1.0f, glm::sin(theta2));
            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));

            // Create a line segment along the Z-axis with the specified color
            m_appSystemsRegistry.getSystem<Renderer>().createLine(point1, point2, color);
        }
    }

    void SceneView::renderCapsule(Transform& transform, glm::vec3 center, float radius, float height, glm::vec4 color) const {
        const int segments = 32; // Number of segments for circles
        const float segmentAngle = glm::two_pi<float>() / static_cast<float>(segments) / 2;

        const float halfHeight = height / 2.0f;
        TransformSystem& transformSystem = m_appSystemsRegistry.getSystem<TransformSystem>();
        Transform tempTransform = transform;
        tempTransform.position += center;
        tempTransform.scale = glm::vec3(std::max(tempTransform.scale.x, std::max(tempTransform.scale.y, tempTransform.scale.z)));
        glm::mat4 modelMatrix = transformSystem.getModelMatrix(tempTransform);
        Renderer& renderer = m_appSystemsRegistry.getSystem<Renderer>();


        for (int i = 0; i < segments; ++i) {
            // Draw top half-sphere along the XY plane
            float theta1 = i * segmentAngle;
            float theta2 = (i + 1) * segmentAngle;

            glm::vec3 point1 = center + glm::vec3(radius * glm::cos(theta1), halfHeight + radius * glm::sin(theta1), 0.0f);
            glm::vec3 point2 = center + glm::vec3(radius * glm::cos(theta2), halfHeight + radius * glm::sin(theta2), 0.0f);

            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));

            // Draw top half-sphere along the YZ plane
            renderer.createLine(point1, point2, color);

            point1 = center + glm::vec3(0.0f, halfHeight + radius * glm::sin(theta1), radius * glm::cos(theta1));
            point2 = center + glm::vec3(0.0f, halfHeight + radius * glm::sin(theta2), radius * glm::cos(theta2));

            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));

            renderer.createLine(point1, point2, color);

            // Draw bottom half-sphere along the XZ plane
            theta1 = i * segmentAngle + glm::radians(180.0f);
            theta2 = (i + 1) * segmentAngle + glm::radians(180.0f);

            point1 = center + glm::vec3(radius * glm::cos(theta1), -halfHeight + radius * glm::sin(theta1), 0.0f);
            point2 = center + glm::vec3(radius * glm::cos(theta2), -halfHeight + radius * glm::sin(theta2), 0.0f);

            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));

            renderer.createLine(point1, point2, color);

            // Draw bottom half-sphere along the YZ plane
            point1 = center + glm::vec3(0.0f, -halfHeight + radius * glm::sin(theta1), radius * glm::cos(theta1));
            point2 = center + glm::vec3(0.0f, -halfHeight + radius * glm::sin(theta2), radius * glm::cos(theta2));

            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));

            renderer.createLine(point1, point2, color);

            // Draw Circles on the top and bottom of the capsule
            theta1 = i * (segmentAngle * 2);
            theta2 = (i + 1) * segmentAngle * 2;

            point1 = center + glm::vec3(radius * glm::cos(theta1), halfHeight, radius * glm::sin(theta1));
            point2 = center + glm::vec3(radius * glm::cos(theta2), halfHeight, radius * glm::sin(theta2));

            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));

            renderer.createLine(point1, point2, color);

            point1 = center + glm::vec3(radius * glm::cos(theta1), -halfHeight, radius * glm::sin(theta1));
            point2 = center + glm::vec3(radius * glm::cos(theta2), -halfHeight, radius * glm::sin(theta2));

            point1 = glm::vec3(modelMatrix * glm::vec4(point1, 1.0f));
            point2 = glm::vec3(modelMatrix * glm::vec4(point2, 1.0f));

            renderer.createLine(point1, point2, color);
        }


        // Draw vertical lines connecting the top and bottom circles
        glm::vec3 topLeft = glm::vec3(modelMatrix * glm::vec4(center + glm::vec3(radius, halfHeight, 0.0f), 1.0f));
        glm::vec3 bottomLeft = glm::vec3(modelMatrix * glm::vec4(center + glm::vec3(radius, -halfHeight, 0.0f), 1.0f));

        glm::vec3 topRight = glm::vec3(modelMatrix * glm::vec4(center + glm::vec3(-radius, halfHeight, 0.0f), 1.0f));
        glm::vec3 bottomRight = glm::vec3(modelMatrix * glm::vec4(center + glm::vec3(-radius, -halfHeight, 0.0f), 1.0f));

        // Draw two vertical lines on the sides
        renderer.createLine(topLeft, bottomLeft, color);
        renderer.createLine(topRight, bottomRight, color);

        topLeft = glm::vec3(modelMatrix * glm::vec4(center + glm::vec3(0.0f, halfHeight, radius), 1.0f));
        bottomLeft = glm::vec3(modelMatrix * glm::vec4(center + glm::vec3(0.0f, -halfHeight, radius), 1.0f));

        topRight = glm::vec3(modelMatrix * glm::vec4(center + glm::vec3(0.0f, halfHeight, -radius), 1.0f));
        bottomRight = glm::vec3(modelMatrix * glm::vec4(center + glm::vec3(0.0f, -halfHeight, -radius), 1.0f));

        renderer.createLine(topLeft, bottomLeft, color);
        renderer.createLine(topRight, bottomRight, color);
    }

    void SceneView::renderCylinder(Transform& transform, glm::vec3 center, float radius, float height, glm::vec4 color) const {
        // Calculate the world space transformation matrix
        TransformSystem& transformSystem = m_appSystemsRegistry.getSystem<TransformSystem>();
        Transform tempTransform = transform;
        tempTransform.position += center;
        glm::mat4 modelMatrix = transformSystem.getModelMatrix(tempTransform);

        const float offset = 0.005f;
        float radiusX = radius + offset;
        float radiusZ = radius + offset;
        height = (0.5f * height) + offset;
        // Calculate the vertices to outline the cylinder
        const int numSegments = 32;
        std::vector<glm::vec3> vertices;
        for (int i = 0; i <= numSegments; ++i) {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(numSegments);
            float x = radiusX * glm::cos(theta);
            float z = radiusZ * glm::sin(theta);

            // Calculate top and bottom points
            glm::vec3 topPoint = glm::vec3(x, height, z);
            glm::vec3 bottomPoint = glm::vec3(x, -height, z);

            // Apply rotation and translation
            topPoint = glm::vec3(modelMatrix * glm::vec4(topPoint, 1.0f));
            bottomPoint = glm::vec3(modelMatrix * glm::vec4(bottomPoint, 1.0f));

            // Add the points to the vertices array
            vertices.push_back(topPoint);
            vertices.push_back(bottomPoint);

            // Draw lines to connect the points
            if (i > 0) {
                glm::vec3 prevTopPoint = vertices[(i - 1) * 2];
                glm::vec3 prevBottomPoint = vertices[(i - 1) * 2 + 1];

                // Render lines for the sides of the cylinder
                m_appSystemsRegistry.getSystem<Renderer>().createLine(prevTopPoint, topPoint, color); // Red lines
                m_appSystemsRegistry.getSystem<Renderer>().createLine(prevBottomPoint, bottomPoint, color); // Red lines

                // Render lines to connect the top and bottom points
                m_appSystemsRegistry.getSystem<Renderer>().createLine(prevTopPoint, prevBottomPoint, color); // Red lines
            }
        }

        // Connect the first and last points to close the cylinder
        glm::vec3 firstTopPoint = vertices[0];
        glm::vec3 firstBottomPoint = vertices[1];
        glm::vec3 lastTopPoint = vertices[vertices.size() - 2];
        glm::vec3 lastBottomPoint = vertices[vertices.size() - 1];

        // Render lines for the sides of the cylinder
        m_appSystemsRegistry.getSystem<Renderer>().createLine(lastTopPoint, firstTopPoint, color); // Red lines
        m_appSystemsRegistry.getSystem<Renderer>().createLine(lastBottomPoint, firstBottomPoint, color); // Red lines

        // Render lines to connect the top and bottom points
        m_appSystemsRegistry.getSystem<Renderer>().createLine(lastTopPoint, lastBottomPoint, color); // Red lines
    }

    glm::vec4 SceneView::getColor(const Tag& tag, bool collider) const {
        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
        const Entity entity = scene.getEntityByTag(tag);

        // Check if entity is selected
        bool isSelected = std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity) != m_selectedEntities.end();

        if (!collider) {
            if (scene.hasComponent<StaticBody>(entity)) {
                return isSelected ? glm::vec4(1.0f, 0.7f, 0.7f, 0.7f) : glm::vec4(0.8f, 0.4f, 0.4f, 0.5f); // Light Red for Static Collider
            }
            if (scene.hasComponent<KinematicBody>(entity)) {
                return isSelected ? glm::vec4(0.7f, 0.9f, 1.0f, 0.7f) : glm::vec4(0.4f, 0.6f, 0.8f, 0.5f); // Sky Blue for Kinematic Collider
            }
            if (scene.hasComponent<RigidBody>(entity)) {
                return isSelected ? glm::vec4(0.7f, 1.0f, 0.7f, 0.7f) : glm::vec4(0.4f, 0.8f, 0.4f, 0.5f); // Light Green for Rigid Collider
            }
        } else {
            if (scene.hasComponent<StaticBody>(entity)) {
                return isSelected ? glm::vec4(0.6f, 0.15f, 0.15f, 0.9f) : glm::vec4(0.4f, 0.08f, 0.08f, 0.7f); // Dark Red for Static Trigger
            }
            if (scene.hasComponent<KinematicBody>(entity)) {
                return isSelected ? glm::vec4(0.3f, 0.5f, 1.0f, 0.9f) : glm::vec4(0.15f, 0.3f, 0.6f, 0.7f); // Dark Blue for Kinematic Trigger
            }
            if (scene.hasComponent<RigidBody>(entity)) {
                return isSelected ? glm::vec4(0.15f, 0.5f, 0.15f, 0.9f) : glm::vec4(0.08f, 0.3f, 0.08f, 0.7f); // Forest Green for Rigid Trigger
            }
        }

        // Default color (should never be reached if entities are well-tagged)
        return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // White
    }
}
