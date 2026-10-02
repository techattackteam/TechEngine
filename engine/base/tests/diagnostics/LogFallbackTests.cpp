#undef TE_LOG_ACTIVE_LEVEL

#include <TechEngine/base/diagnostics/Log.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("a TU without the level define falls back on NDEBUG", "[base][log]") {
#if defined(NDEBUG)
    REQUIRE(TE_LOG_ACTIVE_LEVEL == TE_LOG_LEVEL_INFO);
#else
    REQUIRE(TE_LOG_ACTIVE_LEVEL == TE_LOG_LEVEL_TRACE);
#endif
}
