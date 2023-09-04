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

TechEngine::AppCore *TechEngine::createApp() {
    return new TechEngine::Editor();
}

