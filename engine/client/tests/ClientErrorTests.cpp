#include <TechEngine/client/ClientError.hpp>
#include <TechEngine/testing/ErrorCategoryChecks.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>

using TechEngine::ClientError;

static constexpr std::array<ClientError, 2> ALL_CODES{ClientError::AlreadyStarted, ClientError::RendererStartFailed};

TEST_CASE("every ClientError code converts into the client category", "[client][clienterror]") {
    TechEngineTests::checkErrorCategory<ClientError>(ALL_CODES, TechEngine::clientErrorCategory(), "client");
}
