#include "GLFW.hpp"
#include "Renderer.hpp"
#include "ErrorCatcher.hpp"
#include "components/light/DirectionalLightComponent.hpp"
#include "scene/SceneHelper.hpp"
#include "core/Logger.hpp"
#include "TextureManager.hpp"
#include "core/FileSystem.hpp"

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
    }

    Renderer::~Renderer() {
        for (auto &frameBuffer: frameBuffers) {
            delete frameBuffer;
        }
    }

    void Renderer::renderWithLightPass(Scene &scene) {
        for (auto *light: scene.getLights()) {
            auto *directionLight = light->getComponent<DirectionalLightComponent>();
            shadersManager.getActiveShader()->setUniformMatrix4f("lightSpaceMatrix", directionLight->getProjectionMatrix() * directionLight->getViewMatrix());
            shadersManager.getActiveShader()->setUniformVec3("lightDirection", light->getTransform().getOrientation());
            shadersManager.getActiveShader()->setUniformVec3("lightColor", directionLight->getColor());
            renderGeometryPass(scene, false);
        }
    }

    void Renderer::renderGameObject(GameObject *gameObject, bool shadow) {
        for (auto &pair: gameObject->getChildren()) {
            renderGameObject(pair.second, shadow);
        }
        if (gameObject->hasComponent<MeshRendererComponent>()) {
            shadersManager.getActiveShader()->setUniformMatrix4f("model", gameObject->getModelMatrix());
            auto *meshRenderer = gameObject->getComponent<MeshRendererComponent>();
            flushMeshData(meshRenderer);
            if (!shadow) {
                Material &material = meshRenderer->getMaterial();
                shadersManager.getActiveShader()->setUniformVec3("material.ambient", material.getAmbient());
                shadersManager.getActiveShader()->setUniformVec3("material.diffuse", material.getDiffuse());
                shadersManager.getActiveShader()->setUniformVec3("material.specular", material.getSpecular());
                shadersManager.getActiveShader()->setUniformFloat("material.shininess", material.getShininess());
                shadersManager.getActiveShader()->setUniformBool("material.useTexture", material.getUseTexture());
                if (material.getUseTexture()) {
                    material.getDiffuseTexture()->bind(1);
                    shadersManager.getActiveShader()->setUniformInt("diffuseTexture", 1);
                }
            }

            vertexBuffers[BufferGameObjects]->bind();
            vertexArrays[BufferGameObjects]->bind();
            indicesBuffers[BufferGameObjects]->bind();
            GlCall(glDrawElements(GL_TRIANGLES, meshRenderer->getIndices().size(), GL_UNSIGNED_INT, 0));
        }
    }

    void Renderer::renderGeometryPass(Scene &scene, bool shadow) {
        for (GameObject *gameObject: scene.getGameObjects()) {
            renderGameObject(gameObject, shadow);
        }
    }

    void Renderer::shadowPass(Scene &scene) {
        shadersManager.changeActiveShader("geometry");
        if (scene.isLightingActive()) {
            shadersManager.getActiveShader()->setUniformBool("isLightingActive", true);
            for (GameObject *gameObject: scene.getLights()) {
                DirectionalLightComponent *light = gameObject->getComponent<DirectionalLightComponent>();
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
        }
    }

    void Renderer::geometryPass(Scene &scene) {
        vertexBuffers[BufferGameObjects]->bind();
        vertexArrays[BufferGameObjects]->bind();
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", SceneHelper::mainCamera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", SceneHelper::mainCamera->getViewMatrix());
        shadersManager.getActiveShader()->setUniformVec3("cameraPosition", SceneHelper::mainCamera->getTransform().getPosition());

        if (scene.isLightingActive()) {
            renderWithLightPass(scene);
        } else {
            renderGeometryPass(scene, false);
        }
        vertexBuffers[BufferGameObjects]->unBind();
        vertexArrays[BufferGameObjects]->unBind();
    }

    void Renderer::linePass() {
        vertexBuffers[BufferLines]->bind();
        vertexArrays[BufferLines]->bind();
        shadersManager.changeActiveShader("line");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", SceneHelper::mainCamera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", SceneHelper::mainCamera->getViewMatrix());
        flushLinesData();
        vertexBuffers[BufferLines]->bind();
        vertexArrays[BufferLines]->bind();
        GlCall(glDrawArrays(GL_LINES, 0, lines.size() * 2));
        vertexBuffers[BufferLines]->unBind();
        vertexArrays[BufferLines]->unBind();;
        lines.clear();
    }

    void Renderer::createLine(const glm::vec3 &startPosition, const glm::vec3 &endPosition, const glm::vec4 &color) {
        Line line = Line(startPosition, endPosition, color);
        lines.push_back(line);
    }

    void Renderer::renderPipeline(Scene &scene) {
        if (!SceneHelper::hasMainCamera(scene)) {
            return;
        }
        GlCall(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
        GlCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        geometryPass(scene);
        if (!lines.empty()) {
            linePass();
        }
    }

    void Renderer::renderCustomPipeline(std::vector<GameObject *> &gameObjects) {
        GlCall(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
        GlCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        vertexBuffers[BufferGameObjects]->bind();
        vertexArrays[BufferGameObjects]->bind();
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", SceneHelper::mainCamera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", SceneHelper::mainCamera->getViewMatrix());
        shadersManager.getActiveShader()->setUniformVec3("cameraPosition", SceneHelper::mainCamera->getTransform().getPosition());
        for (GameObject *gameObject: gameObjects) {
            renderGameObject(gameObject, false);
        }
        vertexBuffers[BufferGameObjects]->unBind();
        vertexArrays[BufferGameObjects]->unBind();
    }

    void Renderer::flushMeshData(MeshRendererComponent *meshRenderer) {
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

    FrameBuffer &Renderer::getFramebuffer(uint32_t id) {
        for (auto &frameBuffer: frameBuffers) {
            if (frameBuffer->getID() == id) {
                return *frameBuffer;
            }
        }
        TE_LOGGER_CRITICAL("Framebuffer with id: %d not found", id);
        throw std::runtime_error("Framebuffer not found");
    }

    uint32_t Renderer::createFramebuffer(uint32_t width, uint32_t height) {
        FrameBuffer *frameBuffer = new FrameBuffer();
        frameBuffer->init(frameBuffers.size() + 1, width, height);
        frameBuffers.emplace_back(frameBuffer);
        return frameBuffer->getID();
    }

    ShadersManager &Renderer::getShadersManager() {
        return shadersManager;
    }
}
