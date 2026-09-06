#include "RuntimeApp.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
    class RuntimeProbe : public TechEngine::RuntimeApp {
    public:
        using TechEngine::RuntimeApp::init;
        using TechEngine::RuntimeApp::shouldClose;

        void advanceFrame() {
            update(m_loop.frame());
        }

        TechEngine::Role loopRole() const {
            return m_loop.frame().role;
        }
    };
}

TEST_CASE("runtime composes as a client", "[runtime]") {
    const RuntimeProbe runtime;

    REQUIRE(runtime.loopRole() == TechEngine::Role::Client);
}

TEST_CASE("runtime stops after its 120-frame demo and resets on init", "[runtime]") {
    RuntimeProbe runtime;
    runtime.init();
    for (int i = 0; i < 120; i++) {
        REQUIRE_FALSE(runtime.shouldClose());
        runtime.advanceFrame();
    }
    CHECK(runtime.shouldClose());
    runtime.init();
    CHECK_FALSE(runtime.shouldClose());
}
