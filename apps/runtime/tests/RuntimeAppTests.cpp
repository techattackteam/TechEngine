#include "RuntimeApp.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {
    class RuntimeProbe : public TechEngine::RuntimeApp {
    public:
        TechEngine::Role loopRole() const {
            return m_loop.frame().role;
        }
    };
}

TEST_CASE("runtime composes as a client", "[runtime]") {
    const RuntimeProbe runtime;

    REQUIRE(runtime.loopRole() == TechEngine::Role::Client);
}
