#include "RuntimeServer.hpp"

namespace TechEngine {
    void RuntimeServer::init() {
        server.init();
    }

    void RuntimeServer::update() {
        server.onUpdate();
    }

    void RuntimeServer::fixedUpdate() {
        server.onFixedUpdate();
    }

    void RuntimeServer::destroy() {
        server.destroy();
    }

    Application* createApp() {
        return new RuntimeServer();
    }
}
