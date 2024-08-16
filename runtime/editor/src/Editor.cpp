#include "Editor.hpp"
#include <iostream>

namespace TechEngine {
    void Editor::init() {
        std::cout << "Editor init" << std::endl;
        client.init();
        server.init();
        running = true;
    }

    void Editor::update() {
        std::cout << "Editor update" << std::endl;
        client.onUpdate();
        server.onUpdate();
    }

    void Editor::fixedUpdate() {
        std::cout << "Editor fixedUpdate" << std::endl;
        client.onFixedUpdate();
        server.onFixedUpdate();
    }

    void Editor::destroy() {
        std::cout << "Editor destroy" << std::endl;
        client.destroy();
        server.destroy();
    }

    Application* createApp() {
        return new Editor();
    }
}
