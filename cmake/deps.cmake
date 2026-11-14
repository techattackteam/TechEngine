# Third-party dependencies: the ONE pinned manifest (ADR-008 §4).
#
# Every FetchContent_Declare lives here, GIT_TAG-pinned to a tag/commit; a module
# just names the target it links (in its techengine_module() call). Nothing is
# vendored except glad2 (ADR-008 §4 case 3), see external/ and the note at the end.
#
# Warning levels ride te_warnings for our targets only; dependencies keep their
# own warning settings.
#
# Pins below are the lead's recommendation; adjust deliberately, then treat the tag
# as the source of truth (a floating branch is NOT a pin).

include(FetchContent)

# Allow FetchContent's update step so changes to pinned tags can be fetched.
set(FETCHCONTENT_UPDATES_DISCONNECTED OFF)

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

# --- editor UI --------------------------------------------------------------
# Neither repo is a CMake project, so the library is built here, and only the editor links it.
# imgui_club has no tags, so it pins a full commit, and a commit pin cannot be a shallow clone.
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.93.0-docking
  GIT_SHALLOW    TRUE)
FetchContent_Declare(imgui_club
  GIT_REPOSITORY https://github.com/ocornut/imgui_club.git
  GIT_TAG        b083116866e05e0d4bccb2c7f28d94a76ee4965e)

# --- config serialization (header-only) -------------------------------------
FetchContent_Declare(tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG        v3.4.0
  GIT_SHALLOW    TRUE)

# --- physics ----------------------------------------------------------------
# Jolt's CMake lives in the Build/ subdir. Trim its samples/tests/tools.
# Jolt appends /Zi under GENERATE_DEBUG_SYMBOLS (its default), which ccache refuses to
# cache (separate-PDB side output); every Jolt TU then recompiles each CI run. On MSVC
# we already get embedded debug info from CMAKE_MSVC_DEBUG_INFORMATION_FORMAT (/Z7), so
# drop Jolt's own flag there; Clang (-g) caches fine, so leave it on for the Linux legs.
# Jolt defaults USE_STATIC_MSVC_RUNTIME_LIBRARY ON (/MTd); every other target here is on
# CMake's default /MDd, and mixing CRTs is an LNK2038 the moment the linker pulls a Jolt
# object in. Ours is the majority, so Jolt moves.
if(MSVC)
  set(GENERATE_DEBUG_SYMBOLS OFF CACHE BOOL "" FORCE)
  set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
endif()
# Jolt otherwise exports _HAS_EXCEPTIONS=0 on MSVC, changing STL exception types
# in our consumers while Catch2 and the other libraries retain the normal types.
set(CPP_EXCEPTIONS_ENABLED ON CACHE BOOL "" FORCE)
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

FetchContent_MakeAvailable(glm spdlog glfw imgui imgui_club tomlplusplus Jolt)

# Jolt precompiles Jolt.h for all 138 of its TUs, and ccache refuses every PCH build and use
# under its default sloppiness, so Jolt recompiled in full on every CI run. Without the PCH
# every Jolt TU is a cache hit; a cold build pays about 30 s more.
set_target_properties(Jolt PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

# toml++ defaults TOML_EXCEPTIONS to 1 whenever the compiler has exceptions, and in that mode
# `toml::parse_result` is a plain alias for `toml::table`, so a failure check against it
# compiles, never fires, and `parse()` throws instead. Link this wrapper, never the upstream
# target directly, so no consumer can pick up the throwing mode by forgetting a define.
add_library(te_tomlplusplus INTERFACE)
add_library(TechEngine::tomlplusplus ALIAS te_tomlplusplus)
target_link_libraries(te_tomlplusplus INTERFACE tomlplusplus::tomlplusplus)
target_compile_definitions(te_tomlplusplus INTERFACE TOML_EXCEPTIONS=0)

# The GL backend embeds its own loader, which dlopens libGL on Linux; glad stays private to client.
add_library(TechEngineImGui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
add_library(TechEngine::imgui ALIAS TechEngineImGui)
target_include_directories(TechEngineImGui SYSTEM PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${imgui_club_SOURCE_DIR}/imgui_threaded_rendering)
target_link_libraries(TechEngineImGui PRIVATE glfw ${CMAKE_DL_LIBS})

# --- model import (editor only) ---------------------------------------------
# Three assimp defaults reach past its own targets, so they are off: a GLOBAL ccache launcher
# stacked on CI's own, a cached CMAKE_DEBUG_POSTFIX that renames every library declared after
# it, and on Linux a system zlib that nothing pins.
set(BUILD_SHARED_LIBS                     OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER            ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER             ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER             ON  CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT                      ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS                    OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES                  OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL                        OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL_PDB                    OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_USE_CCACHE               OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX           OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB                     ON  CACHE BOOL "" FORCE)
set(ASSIMP_IGNORE_GIT_HASH                ON  CACHE BOOL "" FORCE)
FetchContent_Declare(assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        v6.0.5
  GIT_SHALLOW    TRUE)
# MikkTSpace is not a CMake project and has no tags, so it pins a full commit and is built here.
FetchContent_Declare(mikktspace
  GIT_REPOSITORY https://github.com/mmikk/MikkTSpace.git
  GIT_TAG        3e895b49d05ea07e4c2133156cfa94369e19e409)
set(MESHOPT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(meshoptimizer
  GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
  GIT_TAG        v1.3
  GIT_SHALLOW    TRUE)
# stb is header-only, has no tags and is not a CMake project, so it pins a full commit. Only its image decoders
# are enabled, and never through its own file IO: the editor decodes from memory.
FetchContent_Declare(stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        9600522de53973336fa033a289858a4354cbb04c)
# bcdec decodes a DDS file's blocks and is header-only. bc7enc_rdo encodes BC5 (rgbcx) and BC7 (the scalar bc7enc).
# Neither has tags, so both pin a full commit. Only bc7enc.cpp and rgbcx.cpp are built: not its CMake project, its
# tools or its ispc encoder: a SOURCE_SUBDIR with no CMakeLists.txt in it makes MakeAvailable fetch without adding
# the repo's own project.
FetchContent_Declare(bcdec
  GIT_REPOSITORY https://github.com/iOrange/bcdec.git
  GIT_TAG        80859ed3b7afb1c527a2a99d70c61457bea72d0c
  SOURCE_SUBDIR  no-cmake-project)
FetchContent_Declare(bc7enc_rdo
  GIT_REPOSITORY https://github.com/richgel999/bc7enc_rdo.git
  GIT_TAG        b9438627eef73a1157e84201b6fa6eb2ffd6d9f0
  SOURCE_SUBDIR  no-cmake-project)
FetchContent_MakeAvailable(assimp mikktspace meshoptimizer stb bcdec bc7enc_rdo)

add_library(TechEngineMikkTSpace STATIC ${mikktspace_SOURCE_DIR}/mikktspace.c)
add_library(TechEngine::mikktspace ALIAS TechEngineMikkTSpace)
target_include_directories(TechEngineMikkTSpace SYSTEM PUBLIC ${mikktspace_SOURCE_DIR})

# stb_image and bcdec are header-only, so each builds its implementation from a one-line file written here. stb's
# defines are PUBLIC: the implementation and every includer must see the same decoder selection.
file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/te_deps/stb_image.cpp
  CONTENT "#define STB_IMAGE_IMPLEMENTATION\n#include <stb_image.h>\n")
add_library(TechEngineStb STATIC ${CMAKE_CURRENT_BINARY_DIR}/te_deps/stb_image.cpp)
add_library(TechEngine::stb ALIAS TechEngineStb)
target_include_directories(TechEngineStb SYSTEM PUBLIC ${stb_SOURCE_DIR})
target_compile_definitions(TechEngineStb PUBLIC STBI_NO_STDIO STBI_ONLY_HDR STBI_ONLY_PNG STBI_ONLY_JPEG STBI_ONLY_TGA)
target_compile_options(TechEngineStb PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>)

file(CONFIGURE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/te_deps/bcdec.cpp
  CONTENT "#define BCDEC_IMPLEMENTATION\n#include <bcdec.h>\n")
add_library(TechEngineBcdec STATIC ${CMAKE_CURRENT_BINARY_DIR}/te_deps/bcdec.cpp)
add_library(TechEngine::bcdec ALIAS TechEngineBcdec)
target_include_directories(TechEngineBcdec SYSTEM PUBLIC ${bcdec_SOURCE_DIR})
target_compile_options(TechEngineBcdec PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>)

add_library(TechEngineBc7Enc STATIC ${bc7enc_rdo_SOURCE_DIR}/bc7enc.cpp ${bc7enc_rdo_SOURCE_DIR}/rgbcx.cpp)
add_library(TechEngine::bc7enc ALIAS TechEngineBc7Enc)
target_include_directories(TechEngineBc7Enc SYSTEM PUBLIC ${bc7enc_rdo_SOURCE_DIR})
target_compile_options(TechEngineBc7Enc PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>)

# --- audio (single-header) --------------------------------------------------
# Not a CMake project: populate the source and wrap the include dir in an
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

# --- profiler (opt-in) ------------------------------------------------------
# Option-guarded like Catch2 below, so a default build fetches nothing. TE_PROFILE is a
# build option and never a shipping runtime flag, ADR-013 §4.
#
# The GIT_TAG is a WIRE-PROTOCOL LOCK, not a version preference: Tracy compiles its
# ProtocolVersion into both sides, so this tag and the Tracy desktop app in use must be
# the same release or the connection is silently refused. Bumping it means re-downloading
# the app (ADR-013 §1). Record the version in B3 - Build & Testing Notes.
#
# No SYSTEM re-export here: Tracy already declares its own include dir SYSTEM
# (its CMakeLists, target_include_directories(TracyClient SYSTEM PUBLIC ...)).
if(TE_PROFILE)
  set(TRACY_ENABLE         ON CACHE BOOL "" FORCE)
  set(TRACY_ON_DEMAND      ON CACHE BOOL "" FORCE)
  set(TRACY_ONLY_LOCALHOST ON CACHE BOOL "" FORCE)
  set(TRACY_NO_BROADCAST   ON CACHE BOOL "" FORCE)
  FetchContent_Declare(tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG        v0.14.1
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(tracy)
endif()

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

# --- glad2 (GL loader): the ONE vendored dep (ADR-008 §4 case 3) ------------
# Generator: glad2 2.0.8 (pip install glad2==2.0.8).
# glad --api gl:core=4.5 --extensions= --out-path external/glad --reproducible c
add_library(TechEngineGlad STATIC ${CMAKE_SOURCE_DIR}/external/glad/src/gl.c)
add_library(TechEngine::glad ALIAS TechEngineGlad)
target_include_directories(TechEngineGlad SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/external/glad/include)
