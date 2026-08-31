#include "RuntimeApp.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("runtime composes as a client", "[runtime]") {
    REQUIRE(TechEngine::runtimeRole() == TechEngine::Role::Client);
}
