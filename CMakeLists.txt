cmake_minimum_required(VERSION 3.20)

project(TechEngine
  DESCRIPTION "Client/server 3D game engine (v2)"
  LANGUAGES CXX)

# --- guard: presets are the intended entry point (ADR-008 §3) ---------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are disabled. Configure with a preset, e.g. "
                      "`cmake --preset windows`.")
endif()

# --- global options ---------------------------------------------------------
option(TE_BUILD_TESTS       "Build the per-module test suites"        ON)
option(TE_ENABLE_CLANG_TIDY "Run clang-tidy on first-party targets"   OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Single output tree so multi-config (VS) and single-config (Ninja) agree.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(TE_ENABLE_CLANG_TIDY)
  find_program(TE_CLANG_TIDY_COMMAND NAMES clang-tidy)
  if(NOT TE_CLANG_TIDY_COMMAND)
    message(FATAL_ERROR "TE_ENABLE_CLANG_TIDY=ON but clang-tidy was not found on PATH.")
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(cmake/warnings.cmake)       # defines te_warnings
include(cmake/sanitizers.cmake)     # augments te_warnings (must follow warnings)
include(cmake/techengine_module.cmake)
if(TE_BUILD_TESTS)
  enable_testing()
endif()

include(cmake/deps.cmake)           # FetchContent manifest (fetches Catch2 under TE_BUILD_TESTS)

if(TE_BUILD_TESTS)
  include(cmake/techengine_test.cmake)
endif()

add_subdirectory(engine)
add_subdirectory(sdk)
add_subdirectory(apps)
