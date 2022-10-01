#include <iostream>
#include "RendererPanel.hpp"

namespace TechEngine {
    RendererPanel::RendererPanel() : Panel("Scene") {
/*        glfwSetWindowUserPointer(window.getHandler(), this);

        glfwSetWindowCloseCallback(window.getHandler(), [](GLFWwindow *handler) {
            TechEngine::dispatchEvent(new WindowCloseEvent((Panel *) (glfwGetWindowUserPointer(handler))));
        });
        glfwSetKeyCallback(window.getHandler(), [](GLFWwindow *handler, int key, int scancode, int action, int mods) {
            Window::windowKeyInput(key, action);
        });
        glfwSetFramebufferSizeCallback(window.getHandler(), [](GLFWwindow *handler, int width, int height) {
            TechEngine::dispatchEvent(new WindowResizeEvent(width, height));
        });*/
        renderer.init(1200, 600);
    }


    void RendererPanel::onUpdate() {
        ImGui::Begin(name.c_str());
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        renderer.getFramebuffer().resize(wsize.x, wsize.y);
        renderer.getFramebuffer().bind();
        renderer.renderPipeline();
        renderer.getFramebuffer().unBind();
        glViewport(0, 0, wsize.x, wsize.y);
        uint64_t textureID = renderer.getFramebuffer().getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void *>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
    }
}
