#include <TechEngine/platform/window/WindowError.hpp>
#include <TechEngine/testing/ErrorCategoryChecks.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>

using TechEngine::WindowError;

static constexpr std::array<WindowError, 4> ALL_CODES{WindowError::AlreadyOpen, WindowError::InvalidSize, WindowError::PlatformInitFailed, WindowError::CreationFailed};

TEST_CASE("every WindowError code converts into the window category", "[window][windowerror]") {
    TechEngineTests::checkErrorCategory<WindowError>(ALL_CODES, TechEngine::windowErrorCategory(), "window");
}
