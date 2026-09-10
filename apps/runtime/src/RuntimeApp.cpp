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

    void RuntimeApp::fixedUpdate(const SimulationContext& frame) {
        TE_LOGGER_INFO("Runtime fixedUpdate: tick {0}, iteration {1}, deltaTime {2}, fixedDeltaTime {3}, alpha {4}, role {5}", frame.tick, frame.iterationIndex, frame.deltaTime, frame.fixedDeltaTime, frame.alpha, toString(frame.role));
    }

    void RuntimeApp::update(const SimulationContext& frame) {
        m_frameCount++;
        TE_LOGGER_INFO("Runtime update: tick {0}, iteration {1}, deltaTime {2}, fixedDeltaTime {3}, alpha {4}, role {5}", frame.tick, frame.iterationIndex, frame.deltaTime, frame.fixedDeltaTime, frame.alpha, toString(frame.role));
        if (m_frameCount >= 120) {
            requestStop();
        }
    }

    void RuntimeApp::shutdown() {
    }

    Role RuntimeApp::runtimeRole() {
        return Role::Client;
    }
}
