#include "RuntimeApp.hpp"

#include "TechEngine/base/diagnostics/Log.hpp"

namespace TechEngine {
    RuntimeApp::RuntimeApp() : App(runtimeRole()) {
    }

    RuntimeApp::~RuntimeApp() {
    }

    void RuntimeApp::init() {
        m_frameCount = 0;
    }

    void RuntimeApp::fixedUpdate(const FrameContext& frame) {
        TE_LOGGER_INFO("Runtime fixedUpdate: tick {0}, frame {1}, deltaTime {2}, fixedDeltaTime {3}, alpha {4}, role {5}", frame.tick, frame.frameIndex, frame.deltaTime, frame.fixedDeltaTime, frame.alpha, toString(frame.role));
    }

    void RuntimeApp::update(const FrameContext& frame) {
        m_frameCount++;
        TE_LOGGER_INFO("Runtime update: tick {0}, frame {1}, deltaTime {2}, fixedDeltaTime {3}, alpha {4}, role {5}", frame.tick, frame.frameIndex, frame.deltaTime, frame.fixedDeltaTime, frame.alpha, toString(frame.role));
    }

    void RuntimeApp::shutdown() {
    }

    bool RuntimeApp::shouldClose() const {
        return m_frameCount >= 120;
    }

    Role RuntimeApp::runtimeRole() {
        return Role::Client;
    }
}
