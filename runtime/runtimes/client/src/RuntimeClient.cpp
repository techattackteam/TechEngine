#include "RuntimeClient.hpp"

namespace TechEngine {
    void RuntimeClient::init() {
        client.init();
    }

    void RuntimeClient::fixedUpdate() {
        client.onFixedUpdate();
    }

    void RuntimeClient::update() {
        client.onUpdate();
    }

    void RuntimeClient::destroy() {
        client.destroy();
    }

    Application* createApp() {
        return new RuntimeClient();
    }
}
