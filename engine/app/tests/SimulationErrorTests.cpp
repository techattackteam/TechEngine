#include <TechEngine/app/SimulationError.hpp>
#include <TechEngine/testing/ErrorCategoryChecks.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>

using TechEngine::SimulationError;

static constexpr std::array<SimulationError, 2> ALL_CODES{SimulationError::AlreadyRunning, SimulationError::StartupFailed};

TEST_CASE("every SimulationError code converts into the simulation category", "[app][simulationerror]") {
    TechEngineTests::checkErrorCategory<SimulationError>(ALL_CODES, TechEngine::simulationErrorCategory(), "simulation");
}
