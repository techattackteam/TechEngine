#include "GLFW.hpp"
#include "Renderer.hpp"
#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"
#include "../scene/Scene.hpp"

namespace Engine {
    void Renderer::init(float width, float height) {
        this->width = width;
        this->height = height;
        vertexArray.init(id);
        vertexBuffer.init(id, 10000000 * sizeof(Vertex));
        vertexArray.addNewBuffer(vertexBuffer);
        shadersManager = ShadersManager();
    }

    void Renderer::renderPipeline() {
        shadersManager.changeActiveShader("geometry");
        shadersManager.getActiveShader()->setUniformMatrix4f("projection", Scene::getInstance().mainCamera->getProjectionMatrix());
        shadersManager.getActiveShader()->setUniformMatrix4f("view", Scene::getInstance().mainCamera->getViewMatrix());
        for (GameObject *gameObject: Scene::getInstance().getGameObjects()) {
            shadersManager.getActiveShader()->setUniformMatrix4f("model", gameObject->getModelMatrix());
        }
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
