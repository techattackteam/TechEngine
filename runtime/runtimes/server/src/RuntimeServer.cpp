#include "RuntimeServer.hpp"

namespace TechEngine {
    void RuntimeServer::init() {
        m_runFunction = [this]() {
            m_server.m_entry.run([this]() {
                                     fixedUpdate();
                                 }, [this]() {
                                     update();
                                 });
        };
        m_server.init();
    }

    void RuntimeServer::start() {
        m_server.onStart();
        m_running = true;
    }

    void RuntimeServer::update() {
        m_server.onUpdate();
    }

    void RuntimeServer::fixedUpdate() {
        m_server.onFixedUpdate();
    }

    void RuntimeServer::stop() {
    }

    void RuntimeServer::shutdown() {
        m_server.destroy();
    }

    Application* createApp() {
        return new RuntimeServer();
    }
}
