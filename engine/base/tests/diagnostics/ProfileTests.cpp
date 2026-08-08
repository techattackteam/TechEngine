#include <TechEngine/base/diagnostics/Profile.hpp>

#include <catch2/catch_test_macros.hpp>

// Nothing to assert on the ON path: the macros evaluate their arguments there by design, and
// the scope macro needs a literal name. Everything below is inside the guard so the profile
// presets — the only builds that define this — do not compile two unreferenced helpers into
// /W4 /WX.
#if !defined(TE_PROFILE_ENABLED)

static int g_sideEffects = 0;

static const char* bumpAndName() {
    g_sideEffects++;
    return "SideEffect";
}

static void* bumpAndPointer() {
    g_sideEffects++;
    return &g_sideEffects;
}

TEST_CASE("Compiled-out profiler macros do not evaluate their arguments", "[profiler]") {
    // Also the only odr-use of the two helpers: the macros below discard their arguments
    // outright, so without this block both are unreferenced and /W4 /WX rejects the file.
    g_sideEffects = 0;
    REQUIRE(bumpAndName() != nullptr);
    REQUIRE(bumpAndPointer() != nullptr);
    REQUIRE(g_sideEffects == 2);

    g_sideEffects = 0;

    TE_PROFILER_SCOPE(bumpAndName());
    TE_PROFILER_FUNCTION();
    TE_PROFILER_FRAME();
    TE_PROFILER_ALLOC(bumpAndPointer(), sizeof(g_sideEffects));
    TE_PROFILER_FREE(bumpAndPointer());

    REQUIRE(g_sideEffects == 0);
}

#endif
