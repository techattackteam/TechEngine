#include "ImGuiPanel.hpp"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"

namespace Engine {
    ImGuiPanel::ImGuiPanel(const std::string &name) : Panel(name, false) {
       // IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window.getHandler(), true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    ImGuiPanel::~ImGuiPanel() {

    }

    void ImGuiPanel::onUpdate() {
        window.takeContext();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Teste");
        ImGui::SameLine();
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        window.onUpdate();
    }
}

