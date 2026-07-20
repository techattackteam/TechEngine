# Third-party dependencies — the ONE pinned manifest (ADR-008 §4).
#
# Every FetchContent_Declare lives here, GIT_TAG-pinned to a tag/commit; a module
# just names the target it links (in its techengine_module() call). Nothing is
# vendored except glad2 (ADR-008 §4 case 3) — see external/ and the note at the end.
#
# Warnings: /WX rides te_warnings (our targets only), so these builds keep their own
# warning settings and are never held to our -Werror.
#
# Pins below are the lead's recommendation; adjust deliberately, then treat the tag
# as the source of truth (a floating branch is NOT a pin).

include(FetchContent)

# Skip the ExternalProject "update" step after the initial download: the GIT_TAG
# checkout already pins us, the re-fetch is redundant, and on Windows/MSBuild that
# step intermittently fails with "The system cannot find the path specified".
# Tradeoff: changing a GIT_TAG needs the dep's _deps dir cleared to re-fetch. CI
# does clean checkouts, so it always fetches the pinned tag fresh.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# --- math (header-only) -----------------------------------------------------
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
  GIT_SHALLOW    TRUE)

# --- logging ----------------------------------------------------------------
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.15.1
  GIT_SHALLOW    TRUE)

# --- window / input ---------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE)

# --- config serialization (header-only) -------------------------------------
FetchContent_Declare(tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG        v3.4.0
  GIT_SHALLOW    TRUE)

# --- physics ----------------------------------------------------------------
# Jolt's CMake lives in the Build/ subdir. Trim its samples/tests/tools.
set(TARGET_UNIT_TESTS       OFF CACHE BOOL "" FORCE)
set(TARGET_HELLO_WORLD      OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES          OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER           OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Jolt
  GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
  GIT_TAG        v5.2.0
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  Build)

FetchContent_MakeAvailable(glm spdlog glfw tomlplusplus Jolt)

# --- audio (single-header) --------------------------------------------------
# Not a CMake project — populate the source and wrap the include dir in an
# INTERFACE target. The MINIAUDIO_IMPLEMENTATION TU lands when client uses audio.
FetchContent_Declare(miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG        0.11.21
  GIT_SHALLOW    TRUE)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
  FetchContent_Populate(miniaudio)
endif()
add_library(miniaudio INTERFACE)
add_library(TechEngine::miniaudio ALIAS miniaudio)
target_include_directories(miniaudio SYSTEM INTERFACE ${miniaudio_SOURCE_DIR})

# --- test framework ---------------------------------------------------------
if(TE_BUILD_TESTS)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.0
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  include(Catch)   # provides catch_discover_tests() for techengine_test()
endif()

# --- glad2 (GL loader) — the ONE vendored dep (ADR-008 §4 case 3) ------------
# Not yet present. glad2 is generated (GL 4.5 core, DSA), not a fetchable CMake
# project, so it is committed under external/glad/ and wrapped in a target here.
# Wired to `platform` once generated — see the open decision in the session notes.
