#include "NewObjectPanel.hpp"

NewObjectPanel::NewObjectPanel() : Engine::CustomPanel("Test panel") {
    counter = 0;
}

void NewObjectPanel::onUpdate() {
    counter++;
    if (ImGui::Button("NEW QUAD MESH ")) {
        new QuadMeshTest(counter);
    }
}
