#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include "GLFW.hpp"
#include "Renderer.hpp"
#include "../scene/Scene.hpp"
#include "ErrorCatcher.hpp"


namespace Engine {
    void Renderer::init(int width, int height) {
        this->width = width;
        this->height = height;
        vertexArray.init(id);
        vertexBuffer.init(id, 10000000 * sizeof(Vertex));
        shadersManager.init();
        shadowMapBuffer.init(id);
        vertexArray.addNewBuffer(vertexBuffer);
        shadowMapBuffer.createDepthTexture(1024, 1024);
    }

    void Renderer::renderPass() {
        for (GameObject *gameObject: Scene::getInstance().getGameObjects()) {
            if (gameObject->hasComponent<MeshRendererComponent>()) {
                shadersManager.getActiveShader()->setUniformMatrix4f("model", gameObject->getModelMatrix());
                auto *meshComponent = gameObject->getComponent<MeshRendererComponent>();
                flushMeshData(meshComponent);
                GlCall(glDrawArrays(GL_TRIANGLES, 0, meshComponent->getMesh().getVertices().size()));
            }
        }
    }

    void Renderer::lightPass() {
        for (GameObject *gameObject: Scene::getInstance().getGameObjects()) {
            if (gameObject->hasComponent<DirectionalLightComponent>()) {
                light = gameObject->getComponent<DirectionalLightComponent>();
                GlCall(glViewport(0, 0, 1024, 1024));
                shadersManager.changeActiveShader("shadowMap");
                shadowMapBuffer.bind();
                shadowMapBuffer.clear();
                shadersManager.getActiveShader()->setUniformMatrix4f("lightSpaceMatrix", light->getProjectionMatrix() * light->getViewMatrix());
                renderPass();
                shadowMapBuffer.unBind();
            }
        }
    }

    void Renderer::geometryPass() {
        GlCall(glViewport(0, 0, width, height));
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", Scene::getInstance().mainCamera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", Scene::getInstance().mainCamera->getViewMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("lightSpaceMatrix", light->getProjectionMatrix() * light->getViewMatrix());
        shadersManager.getActiveShader()->setUniformVec3("lightDirection", Scene::getInstance().mainCamera->getTransform().getPosition());
        shadersManager.getActiveShader()->setUniformVec3("cameraPosition", Scene::getInstance().mainCamera->getTransform().getPosition());
        GlCall(glClearColor(0.2f, 0.2f, 0.2f, 1.0f));
        GlCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        shadowMapBuffer.bindShadowMapTexture();
        renderPass();
    }

    void Renderer::renderPipeline() {
        if (!Scene::getInstance().hasMainCamera()) {
            return;
        }

        vertexBuffer.bind();
        vertexArray.bind();

        lightPass();
        geometryPass();

        vertexBuffer.unBind();
        vertexArray.unBind();
    }

    void Renderer::flushMeshData(MeshRendererComponent *meshComponent) {
        vertexBuffer.addData(meshComponent->getMesh().getVertices().data(), meshComponent->getMesh().getVertices().size() * sizeof(Vertex), 0);
    }

    void Renderer::beginImGuiFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Renderer::ImGuiPipeline() const {
        // Rendering
        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO &io = ImGui::GetIO();
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    unsigned int quadVAO = 0;
    unsigned int quadVBO;

    void Renderer::renderQuad() {
        if (quadVAO == 0) {
            float quadVertices[] = {
                    // positions        // texture Coords
                    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
                    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                    1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            };
            // setup plane VAO
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
        }
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

}
