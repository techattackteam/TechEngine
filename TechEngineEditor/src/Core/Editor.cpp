#include "Editor.hpp"
#include "testGameObject/QuadMeshTest.hpp"

namespace TechEngine {

    Editor::Editor() {
        new QuadMeshTest(1);
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
