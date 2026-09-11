#include <RuntimeApp.hpp>

namespace TechEngine {
    RuntimeApp::RuntimeApp() : App(runtimeRole()) {
    }
    void RuntimeApp::init() {
    }
    void RuntimeApp::fixedUpdate(const SimulationContext& simulation) {
        if (simulation.tick >= 120) {
            requestStop();
        }
    }
    Role RuntimeApp::runtimeRole() {
        return Role::Client;
    }
}
