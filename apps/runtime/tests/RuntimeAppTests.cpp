#include "RuntimeApp.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
    class RuntimeProbe : public TechEngine::RuntimeApp {
    public:
        using TechEngine::RuntimeApp::init;
        using TechEngine::RuntimeApp::stopRequested;

        void advanceFrame() {
            update(m_simulationThread.simulationContext());
        }

        TechEngine::Role loopRole() const {
            return m_simulationThread.simulationContext().role;
        }
    };
}

TEST_CASE("runtime composes as a client", "[runtime]") {
    const RuntimeProbe runtime;

    REQUIRE(runtime.loopRole() == TechEngine::Role::Client);
}

TEST_CASE("runtime requests a stop after its 120-frame demo", "[runtime]") {
    RuntimeProbe runtime;
    runtime.init();
    for (int i = 0; i < 120; i++) {
        REQUIRE_FALSE(runtime.stopRequested());
        runtime.advanceFrame();
    }
    CHECK(runtime.stopRequested());
}
