#include <TechEngine/base/Base.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <string_view>

TEST_CASE("base version anchor is present", "[base]") {
    REQUIRE(std::string_view{TechEngine::baseVersion()} == "0.0.0");
}

TEST_CASE("glm links and computes through te_base", "[base][glm]") {
    const glm::vec3 v{1.0f, 2.0f, 2.0f};
    REQUIRE(glm::length(v) == Catch::Approx(3.0f));
}
