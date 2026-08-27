# Preset-driven source-based coverage (S4-P3).
#
# A configure preset sets TE_COVERAGE=ON; the flags are added to te_warnings so they
# reach our targets only, never FetchContent deps. Clang/Linux only, which is the leg
# ADR-005 already puts clang-tidy and the Linux sanitizers on.
#
# Must be included AFTER warnings.cmake (it mutates te_warnings).

if(NOT TARGET te_warnings)
  message(FATAL_ERROR "coverage.cmake requires te_warnings (include warnings.cmake first)")
endif()

option(TE_COVERAGE "Instrument first-party targets for llvm-cov coverage" OFF)

if(NOT TE_COVERAGE)
  return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "TE_COVERAGE requires Clang (got '${CMAKE_CXX_COMPILER_ID}'); "
                      "coverage lives on the Linux/Clang leg.")
endif()

target_compile_options(te_warnings INTERFACE -fprofile-instr-generate -fcoverage-mapping)
target_link_options(te_warnings INTERFACE -fprofile-instr-generate)

# GOTCHA: llvm-profdata/llvm-cov must match the Clang major version that compiled, or the
# merge fails with an unsupported-profile-version error. Searching the compiler's own bin
# directory first is what keeps them aligned when several LLVM versions are installed.
get_filename_component(_te_llvm_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
find_program(TE_LLVM_PROFDATA NAMES llvm-profdata HINTS "${_te_llvm_bin}")
find_program(TE_LLVM_COV NAMES llvm-cov HINTS "${_te_llvm_bin}")

if(NOT TE_LLVM_PROFDATA OR NOT TE_LLVM_COV)
  message(FATAL_ERROR "TE_COVERAGE needs llvm-profdata and llvm-cov (apt package 'llvm').")
endif()

# Pinned, and checked here rather than at report time. diff-cover decides whether a merge
# is allowed, and its CLI changed shape between releases: the apt package still wants
# --markdown-report FILENAME where this expects --format markdown:PATH. A local run that
# does not predict CI is worse than no local run, so the versions have to match.
set(TE_DIFF_COVER_VERSION "10.5.1")

find_program(TE_DIFF_COVER NAMES diff-cover)
if(NOT TE_DIFF_COVER)
  message(FATAL_ERROR
          "TE_COVERAGE needs diff-cover ${TE_DIFF_COVER_VERSION} on PATH.
"
          "  sudo apt install -y pipx
"
          "  pipx install diff-cover==${TE_DIFF_COVER_VERSION}
"
          "  pipx ensurepath   # then open a new shell: ~/.local/bin must be on PATH")
endif()

execute_process(COMMAND "${TE_DIFF_COVER}" --version
                OUTPUT_VARIABLE _te_dc_version OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET RESULT_VARIABLE _te_dc_result)

if(NOT _te_dc_result EQUAL 0 OR NOT _te_dc_version MATCHES "${TE_DIFF_COVER_VERSION}")
  message(FATAL_ERROR
          "diff-cover at ${TE_DIFF_COVER} reports '${_te_dc_version}', expected "
          "${TE_DIFF_COVER_VERSION} (the version CI pins).
"
          "  sudo apt remove diff-cover   # the distro package shadows the pinned one
"
          "  pipx install diff-cover==${TE_DIFF_COVER_VERSION}")
endif()

message(STATUS "TechEngine: coverage instrumentation enabled on first-party targets")

# Called from the bottom of the top-level CMakeLists, once every add_subdirectory has run
# and TE_TEST_TARGETS is complete.
function(te_add_coverage_target)
  get_property(_te_tests GLOBAL PROPERTY TE_TEST_TARGETS)

  if(NOT _te_tests)
    message(FATAL_ERROR "TE_COVERAGE is ON but no test targets registered "
                        "(TE_BUILD_TESTS is probably OFF).")
  endif()

  # llvm-cov takes the first binary positionally and every other one behind -object.
  set(_te_objects "")
  foreach(_te_test IN LISTS _te_tests)
    list(APPEND _te_objects "$<TARGET_FILE:${_te_test}>")
  endforeach()

  set(_te_dir "${CMAKE_BINARY_DIR}/coverage")

  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${_te_dir}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_te_dir}"
    COMMAND ${CMAKE_COMMAND} -E env "LLVM_PROFILE_FILE=${_te_dir}/%m.profraw"
            ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${CMAKE_COMMAND}
            -DTE_COV_DIR=${_te_dir}
            -DTE_LLVM_PROFDATA=${TE_LLVM_PROFDATA}
            -DTE_LLVM_COV=${TE_LLVM_COV}
            -DTE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DTE_DIFF_COVER=${TE_DIFF_COVER}
            "-DTE_OBJECTS=$<JOIN:${_te_objects},|>"
            -P "${CMAKE_SOURCE_DIR}/cmake/coverage_report.cmake"
    DEPENDS ${_te_tests}
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Running tests under instrumentation, then reporting diff coverage"
    USES_TERMINAL VERBATIM)
endfunction()
