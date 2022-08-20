#include "NewObjectPanel.hpp"

NewObjectPanel::NewObjectPanel() : TechEngine::CustomPanel("Test panel") {
    counter = 0;
}

void NewObjectPanel::onUpdate() {
    ImGui::SetCurrentContext(imGuiContext);
    ImGui::Begin(name.c_str());
    if (ImGui::Button("NEW QUAD MESH ")) {
        new QuadMeshTest(counter);
        counter++;
    }
    ImGui::End();
}
