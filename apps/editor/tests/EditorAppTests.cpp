#include "EditorApp.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("editor hosts a client", "[editor]") {
    REQUIRE(TechEngine::editorRole() == TechEngine::Role::Client);
}
