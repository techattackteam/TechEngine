#include "GLFW.hpp"
#include "Renderer.hpp"
#include "ErrorCatcher.hpp"
#include "RendererSettings.hpp"
#include "components/DirectionalLightComponent.hpp"
#include "scene/SceneHelper.hpp"

namespace TechEngine {
    void Renderer::init() {
        vertexArray.init(id);
        vertexBuffer.init(id, 10000000 * sizeof(Vertex));
        shadersManager.init();
        frameBuffer.init(id, RendererSettings::width, RendererSettings::height);
        vertexArray.addNewBuffer(vertexBuffer);
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
        GlCall(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
        GlCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        if (scene.isLightingActive()) {
            renderWithLightPass();
        } else {
            renderGeometryPass(false);
        }


    }

    void Renderer::renderPipeline() {
        if (!SceneHelper::hasMainCamera()) {
            return;
        }
        vertexBuffer.bind();
        vertexArray.bind();

        geometryPass();

        vertexBuffer.unBind();
        vertexArray.unBind();
    }

    void Renderer::flushMeshData(MeshRendererComponent *meshRenderer) {
        vertexBuffer.addData(meshRenderer->getVertices().data(), meshRenderer->getVertices().size() * sizeof(Vertex), 0);
        //shadersManager.getActiveShader()->setUniformVec4("material.color", material.getColor());
    }

    FrameBuffer &Renderer::getFramebuffer() {
        return frameBuffer;
    }
}
