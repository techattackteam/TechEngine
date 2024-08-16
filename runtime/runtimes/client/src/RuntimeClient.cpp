#include "RuntimeClient.hpp"

namespace TechEngine {
    void RuntimeClient::init() {
        m_runFunction = [this]() {
            m_client.m_entry.run([this]() {
                                     fixedUpdate();
                                 }, [this]() {
                                     update();
                                 });
        };
        m_client.init();
    }

    void RuntimeClient::start() {
        m_client.onStart();
        m_running = true;
    }

    void RuntimeClient::fixedUpdate() {
        m_client.onFixedUpdate();
    }

    void RuntimeClient::update() {
        m_client.onUpdate();
    }

    void RuntimeClient::stop() {
        m_client.onStop();
    }

    void RuntimeClient::destroy() {
        m_client.destroy();
    }

    Application* createApp() {
        return new RuntimeClient();
    }
}
