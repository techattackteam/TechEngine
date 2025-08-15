#include "GLFW.hpp"
#include "Renderer.hpp"

#include <any>

#include "ErrorCatcher.hpp"
#include "FrameBuffer.hpp"
#include "IndicesBuffer.hpp"
#include "Line.hpp"
#include "VertexArray.hpp"
#include "core/Logger.hpp"
#include "resources/material/Material.hpp"
#include "resources/mesh/Vertex.hpp"
#include "scene/ScenesManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/TextWidget.hpp"

namespace TechEngine {
    void Renderer::init() {
        shadersManager.init();
        vertexArrays[BufferGameObjects] = new VertexArray();
        vertexBuffers[BufferGameObjects] = new VertexBuffer();
        vertexArrays[BufferGameObjects]->init();
        vertexBuffers[BufferGameObjects]->init(10000000 * sizeof(Vertex));
        vertexArrays[BufferGameObjects]->addNewBuffer(*vertexBuffers[BufferGameObjects]);
        indicesBuffers[BufferGameObjects] = new IndicesBuffer();
        indicesBuffers[BufferGameObjects]->init(10000000 * sizeof(uint32_t));

        vertexArrays[BufferLines] = new VertexArray();
        vertexBuffers[BufferLines] = new VertexBuffer();
        vertexArrays[BufferLines]->init();
        vertexBuffers[BufferLines]->init(10000000 * sizeof(Line));
        vertexArrays[BufferLines]->addNewLinesBuffer(*vertexBuffers[BufferLines]);
        glEnable(GL_DEPTH_TEST);
        uiRenderer.init();
    }

    void Renderer::shutdown() {
        System::shutdown();
        uiRenderer.shutdown();
    }

    Renderer::Renderer(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry), shadersManager(systemsRegistry), uiRenderer(m_systemsRegistry) {
    }

    Renderer::~Renderer() {
        for (auto& frameBuffer: frameBuffers) {
            delete frameBuffer;
        }
    }

    void Renderer::renderWithLightPass(Scene& scene) {
        /*for (auto* light: scene.getLights()) {
            auto* directionLight = light->getComponent<DirectionalLightComponent>();
            shadersManager.getActiveShader()->setUniformMatrix4f("lightSpaceMatrix", directionLight->getProjectionMatrix() * directionLight->getViewMatrix());
            shadersManager.getActiveShader()->setUniformVec3("lightDirection", light->getTransform().getOrientation());
            shadersManager.getActiveShader()->setUniformVec3("lightColor", directionLight->getColor());
            renderGeometryPass(scene, false);
        }*/
    }

    void Renderer::renderMesh(Transform& transform, MeshRenderer& meshRenderer, bool shadow) {
        /*for (auto& pair: gameObject->getChildren()) {
            renderGameObject(pair.second, shadow);
        }*/
        glm::mat4 model = transform.getModelMatrix();
        shadersManager.getActiveShader()->setUniformMatrix4f("model", model);
        flushMeshData(&meshRenderer);
        if (!shadow) {
            Material& material = meshRenderer.material;
            /*
            shadersManager.getActiveShader()->setUniformVec3("material.ambient", material.getAmbient());
            shadersManager.getActiveShader()->setUniformVec3("material.diffuse", material.getDiffuse());
            shadersManager.getActiveShader()->setUniformVec3("material.specular", material.getSpecular());
            shadersManager.getActiveShader()->setUniformFloat("material.shininess", material.getShininess());
            */
            shadersManager.getActiveShader()->setUniformBool("material.useTexture", material.getUseTexture());
            /*if (material.getUseTexture()) {
                material.getDiffuseTexture()->bind(1);
                shadersManager.getActiveShader()->setUniformInt("diffuseTexture", 1);
            }*/
        }

        vertexBuffers[BufferGameObjects]->bind();
        vertexArrays[BufferGameObjects]->bind();
        indicesBuffers[BufferGameObjects]->bind();
        GlCall(glDrawElements(GL_TRIANGLES, meshRenderer.getIndices().size(), GL_UNSIGNED_INT, 0));
    }

    void Renderer::renderGeometryPass(Scene& scene, bool shadow) {
        scene.runSystem<Transform, MeshRenderer>([this, shadow](Transform& transform, MeshRenderer& meshRenderer) {
            if (shadow) {
                shadersManager.changeActiveShader("shadowMap");
            } else {
                shadersManager.changeActiveShader("geometry");
            }
            renderMesh(transform, meshRenderer, shadow);
        });
    }

    void Renderer::shadowPass(Scene& scene) {
        /*shadersManager.changeActiveShader("geometry");
        if (scene.isLightingActive()) {
            shadersManager.getActiveShader()->setUniformBool("isLightingActive", true);
            for (GameObject* gameObject: scene.getLights()) {
                DirectionalLightComponent* light = gameObject->getComponent<DirectionalLightComponent>();
                GlCall(glViewport(0, 0, 1024, 1024));
                shadersManager.changeActiveShader("shadowMap");
                //shadowMapBuffer.bind();
                //shadowMapBuffer.clear();
                shadersManager.getActiveShader()->setUniformMatrix4f("lightSpaceMatrix", light->getProjectionMatrix() * light->getViewMatrix());
                renderGeometryPass(scene, true);
                //shadowMapBuffer.unBind();
            }
        } else {
            shadersManager.getActiveShader()->setUniformBool("isLightingActive", false);
        }*/
    }

    void Renderer::geometryPass(Camera& camera) {
        vertexBuffers[BufferGameObjects]->bind();
        vertexArrays[BufferGameObjects]->bind();
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera.getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", camera.getViewMatrix());
        //shadersManager.getActiveShader()->setUniformVec3("cameraPosition", camera->getTransform().getPosition());

        renderGeometryPass(m_systemsRegistry.getSystem<ScenesManager>().getActiveScene(), false);
        vertexBuffers[BufferGameObjects]->unBind();
        vertexArrays[BufferGameObjects]->unBind();
    }

    void Renderer::uiPass() {
        uiRenderer.beginFrame();
        PanelWidget rootWidget;
        rootWidget.m_name = "Root";
        // For this test, we'll manually set its screen rect to the full viewport
        rootWidget.m_finalScreenRect = {0.0f, 0.0f, (float)uiRenderer.m_screenWidth, (float)uiRenderer.m_screenHeight};


        // 2. Create a Panel to test drawing and absolute positioning
        PanelWidget myPanel;
        myPanel.m_backgroundColor = {1.0f, 0.0f, 0.0f, 1.0f}; // Red
        // Manually set its final rect for this test.
        // In Milestone 2, calculateLayout() will do this for us.
        myPanel.m_finalScreenRect = {100.0f, 100.0f, 200.0f, 150.0f};


        // 3. Create a Text Widget
        TextWidget myLabel;
        myLabel.m_text = "Hello, World!";
        myLabel.m_textColor = {1.0f, 1.0f, 1.0f, 1.0f}; // White
        // Manually set its position
        myLabel.m_finalScreenRect = {110.0f, 150.0f, 180.0f, 30.0f};


        // 4. Create another panel to test batching
        PanelWidget greenPanel;
        greenPanel.m_backgroundColor = {0.0f, 1.0f, 0.0f, 0.5f}; // Semi-transparent green
        greenPanel.m_finalScreenRect = {400.0f, 200.0f, 50.0f, 50.0f};

        // 5. Create more text to test font rendering with the same atlas
        TextWidget anotherLabel;
        anotherLabel.m_text = "This is a Test.";
        anotherLabel.m_textColor = {1.0f, 1.0f, 0.0f, 1.0f}; // Yellow
        anotherLabel.m_finalScreenRect = {400.0f, 300.0f, 150.0f, 30.0f};


        // 6. Draw the widgets
        // We are manually calling draw. In the future, a single call
        // to rootWidget.draw() will handle the entire hierarchy.
        myPanel.draw(uiRenderer);
        myLabel.draw(uiRenderer);
        greenPanel.draw(uiRenderer);
        anotherLabel.draw(uiRenderer);

        uiRenderer.endFrame();
    }

    void Renderer::linePass(Camera& camera) {
        vertexBuffers[BufferLines]->bind();
        vertexArrays[BufferLines]->bind();
        shadersManager.changeActiveShader("line");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera.getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", camera.getViewMatrix());
        flushLinesData();
        vertexBuffers[BufferLines]->bind();
        vertexArrays[BufferLines]->bind();
        GlCall(glDrawArrays(GL_LINES, 0, lines.size() * 2));
        vertexBuffers[BufferLines]->unBind();
        vertexArrays[BufferLines]->unBind();;
        lines.clear();
    }

    void Renderer::createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color) {
        Line line = Line(startPosition, endPosition, color);
        lines.push_back(line);
    }

    void Renderer::renderPipeline(Camera& camera) {
        GlCall(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
        GlCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        geometryPass(camera);
        if (!lines.empty()) {
            linePass(camera);
        }
        uiPass();
    }

    void Renderer::renderCustomPipeline(Camera* camera, std::vector<int>& entities) {
        GlCall(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
        GlCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        vertexBuffers[BufferGameObjects]->bind();
        vertexArrays[BufferGameObjects]->bind();
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", camera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", camera->getViewMatrix());
        //shadersManager.getActiveShader()->setUniformVec3("cameraPosition", camera->getTransform().getPosition());
        auto& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, MeshRenderer>([this](Transform& transform, MeshRenderer& meshRenderer) {
            renderMesh(transform, meshRenderer, false);
        });
        vertexBuffers[BufferGameObjects]->unBind();
        vertexArrays[BufferGameObjects]->unBind();
    }

    void Renderer::flushMeshData(MeshRenderer* meshRenderer) {
        vertexBuffers[BufferGameObjects]->addData(meshRenderer->getVertices().data(), meshRenderer->getVertices().size() * sizeof(Vertex), 0);
        indicesBuffers[BufferGameObjects]->addData(meshRenderer->getIndices().data(), meshRenderer->getIndices().size() * sizeof(uint32_t));
    }

    void Renderer::flushLinesData() {
        std::vector<LineVertex> vertices;
        for (Line line: lines) {
            vertices.push_back(line.getStart());
            vertices.push_back(line.getEnd());
        }
        vertexBuffers[BufferLines]->addData(vertices.data(), vertices.size() * sizeof(LineVertex), 0);
    }

    FrameBuffer& Renderer::getFramebuffer(uint32_t id) {
        for (auto& frameBuffer: frameBuffers) {
            if (frameBuffer->getID() == id) {
                return *frameBuffer;
            }
        }
        TE_LOGGER_CRITICAL("Framebuffer with id: %d not found", id);
        throw std::runtime_error("Framebuffer not found");
    }

    uint32_t Renderer::createFramebuffer(uint32_t width, uint32_t height) {
        FrameBuffer* frameBuffer = new FrameBuffer();
        frameBuffer->init(frameBuffers.size() + 1, width, height);
        frameBuffers.emplace_back(frameBuffer);
        return frameBuffer->getID();
    }

    ShadersManager& Renderer::getShadersManager() {
        return shadersManager;
    }

    UIRenderer& Renderer::getUIRenderer() {
        return uiRenderer;
    }
}
