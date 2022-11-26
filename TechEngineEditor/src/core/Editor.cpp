#include "Editor.hpp"

namespace TechEngine {

    Editor::Editor() {
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
