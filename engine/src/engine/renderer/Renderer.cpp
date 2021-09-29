#include "GLFW.hpp"
#include "Renderer.hpp"
#include "../scene/Scene.hpp"
#include "ErrorCatcher.hpp"


namespace Engine {
    void Renderer::init(float width, float height) {
        this->width = width;
        this->height = height;
        vertexArray.init(id);
        vertexBuffer.init(id, 10000000 * sizeof(Vertex));
        vertexArray.addNewBuffer(vertexBuffer);
        shadersManager.init();

    }

    void Renderer::renderPipeline() {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", Scene::getInstance().mainCamera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", Scene::getInstance().mainCamera->getViewMatrix());
        vertexBuffer.bind();
        vertexArray.bind();

        for (GameObject *gameObject: Scene::getInstance().getGameObjects()) {
            if (gameObject->hasComponent<MeshRendererComponent>()) {
                shadersManager.getActiveShader()->setUniformMatrix4f("model", gameObject->getModelMatrix());
                auto *meshComponent = gameObject->getComponent<MeshRendererComponent>();
                flushMeshData(meshComponent);
                GlCall(glDrawArrays(GL_TRIANGLES, 0, meshComponent->getMesh().getVertices().size()));
            }
        }
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
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        ImGuiIO &io = ImGui::GetIO();
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

}
