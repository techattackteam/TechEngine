#include "Editor.hpp"

namespace TechEngine {

    Editor::Editor() {
        //new QuadMeshTest("UpdateEntity");
        new QuadMeshTest("FixedUpdateEntity");
    }

    void Editor::onUpdate() {
        panelsManager.update();
        window.onUpdate();
    }

    void Editor::onFixedUpdate() {

    }

}

TechEngineCore::App *TechEngineCore::createApp() {
    return new TechEngine::Editor();
}
