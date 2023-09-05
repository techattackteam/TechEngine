#include "GLFW.hpp"
#include "Renderer.hpp"
#include "ErrorCatcher.hpp"
#include "components/DirectionalLightComponent.hpp"
#include "scene/SceneHelper.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    void Renderer::init() {
        vertexArray.init(id);
        vertexBuffer.init(id, 10000000 * sizeof(Vertex));
        shadersManager.init();
        vertexArray.addNewBuffer(vertexBuffer);
    }

    Renderer::~Renderer() {
        for (auto &frameBuffer: frameBuffers) {
            delete frameBuffer;
        }
    }

    void Renderer::renderWithLightPass() {
        for (auto *light: scene.getLights()) {
            auto *directionLight = light->getComponent<DirectionalLightComponent>();
            shadersManager.getActiveShader()->setUniformMatrix4f("lightSpaceMatrix", directionLight->getProjectionMatrix() * directionLight->getViewMatrix());
            shadersManager.getActiveShader()->setUniformVec3("lightDirection", light->getTransform().getOrientation());
            shadersManager.getActiveShader()->setUniformVec3("lightColor", directionLight->getColor());
            renderGeometryPass(false);
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
            }
            GlCall(glDrawArrays(GL_TRIANGLES, 0, meshRenderer->getVertices().size()));
        }
    }

    void Renderer::renderGeometryPass(bool shadow) {
        for (GameObject *gameObject: scene.getGameObjects()) {
            renderGameObject(gameObject, shadow);
        }
    }

    void Renderer::shadowPass() {
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
                renderGeometryPass(true);
                //shadowMapBuffer.unBind();
            }
        } else {
            shadersManager.getActiveShader()->setUniformBool("isLightingActive", false);
        }
    }

    void Renderer::geometryPass() {
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", SceneHelper::mainCamera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", SceneHelper::mainCamera->getViewMatrix());
        shadersManager.getActiveShader()->setUniformVec3("cameraPosition", SceneHelper::mainCamera->getTransform().getPosition());

        if (scene.isLightingActive()) {
            renderWithLightPass();
        } else {
            renderGeometryPass(false);
        }
    }

    void Renderer::renderLine(const glm::vec3 &startPosition, const glm::vec3 &endPosition, const glm::vec3 &color) {
        shadersManager.changeActiveShader("line");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", SceneHelper::mainCamera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", SceneHelper::mainCamera->getViewMatrix());
        shadersManager.getActiveShader()->setUniformVec3("start", startPosition);
        shadersManager.getActiveShader()->setUniformVec3("end", endPosition);
        shadersManager.getActiveShader()->setUniformVec3("color", color);

        GlCall(glDrawArrays(GL_LINES, 0, 2));
    }

    void Renderer::renderPipeline() {
        if (!SceneHelper::hasMainCamera()) {
            return;
        }
        vertexBuffer.bind();
        vertexArray.bind();
        GlCall(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
        GlCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        geometryPass();

        vertexBuffer.unBind();
        vertexArray.unBind();
    }

    void Renderer::flushMeshData(MeshRendererComponent *meshRenderer) {
        vertexBuffer.addData(meshRenderer->getVertices().data(), meshRenderer->getVertices().size() * sizeof(Vertex), 0);
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
}
