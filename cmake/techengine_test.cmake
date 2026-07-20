# techengine_test(<module>
#   SOURCES <explicit .cpp list>)   # REQUIRED — no GLOB
#
# Mirrors techengine_module() for a per-module Catch2 exe (ADR-008 §6): builds
# TechEngine<Module>Tests, links the module + Catch2 + te_warnings, and registers
# each case with CTest via catch_discover_tests. Per-module exes — never one
# monolithic test binary (that reintroduces a god-target, ADR-008 §6).
#
# Only defined/used under TE_BUILD_TESTS (Catch2 is fetched behind the same flag).

function(techengine_test module)
  cmake_parse_arguments(TT "" "" "SOURCES" ${ARGN})

  if(NOT TT_SOURCES)
    message(FATAL_ERROR "techengine_test(${module}): SOURCES is required (explicit list, no GLOB).")
  endif()

  # PascalCase the module dir name: base -> TechEngineBaseTests (matches
  # techengine_module()'s TechEngine<Module> convention).
  string(SUBSTRING "${module}" 0 1 _te_first)
  string(TOUPPER "${_te_first}" _te_first)
  string(SUBSTRING "${module}" 1 -1 _te_rest)
  set(_te_target "TechEngine${_te_first}${_te_rest}Tests")

  add_executable(${_te_target} ${TT_SOURCES})
  target_link_libraries(${_te_target} PRIVATE
    TechEngine::${module}
    Catch2::Catch2WithMain
    te_warnings)

  set_target_properties(${_te_target} PROPERTIES FOLDER "tests")

  catch_discover_tests(${_te_target})
endfunction()
