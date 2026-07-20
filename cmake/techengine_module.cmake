# techengine_module(<name>
#   SOURCES       <explicit .cpp list>        # REQUIRED — no GLOB (ADR-008 §2, kills F6)
#   [HEADERS      <headers>]                  # for IDE grouping only
#   [DEPS         <our modules>]              # PUBLIC  link TechEngine::<m>
#   [DEPS_PRIVATE <our modules>]              # PRIVATE link TechEngine::<m>
#   [LIBS         <3rd-party targets>]        # PUBLIC  3rd-party
#   [LIBS_PRIVATE <3rd-party targets>])       # PRIVATE 3rd-party
#
# The ONE helper that stamps out a static-lib module (ADR-008 §2). It encodes the
# invariants that must be identical in every module so drift is impossible:
#   - explicit sources (the helper never globs)
#   - PUBLIC include / PRIVATE src  (the ADR-006 §3 boundary, made physical)
#   - te_warnings linkage, C++20, the TechEngine:: alias
#   - opt-in clang-tidy
#
# Naming (B4 — Code Conventions): the real target is PascalCase `TechEngine<Module>`
# (v1 nomenclature, e.g. TechEngineBase), refining the illustrative `te_<module>`
# spelling in ADR-008 §2. Consumers link the `TechEngine::<module>` alias, never the
# real name — so the target name is an internal/IDE-facing detail.
#
# Visibility rule (ADR-008 §8): PUBLIC if the dep appears in this module's public
# headers, PRIVATE if only in its .cpp.

function(techengine_module name)
  cmake_parse_arguments(TM "" ""
    "SOURCES;HEADERS;DEPS;DEPS_PRIVATE;LIBS;LIBS_PRIVATE" ${ARGN})

  if(NOT TM_SOURCES)
    message(FATAL_ERROR "techengine_module(${name}): SOURCES is required (explicit list, no GLOB).")
  endif()

  # PascalCase the module dir name into the real target: base -> TechEngineBase.
  string(SUBSTRING "${name}" 0 1 _te_first)
  string(TOUPPER "${_te_first}" _te_first)
  string(SUBSTRING "${name}" 1 -1 _te_rest)
  set(_te_target "TechEngine${_te_first}${_te_rest}")

  add_library(${_te_target} STATIC ${TM_SOURCES} ${TM_HEADERS})
  add_library(TechEngine::${name} ALIAS ${_te_target})

  target_include_directories(${_te_target}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

  target_compile_features(${_te_target} PUBLIC cxx_std_20)

  # Map our-module names to their TechEngine:: aliases.
  set(_te_pub_deps "")
  foreach(dep IN LISTS TM_DEPS)
    list(APPEND _te_pub_deps TechEngine::${dep})
  endforeach()
  set(_te_priv_deps "")
  foreach(dep IN LISTS TM_DEPS_PRIVATE)
    list(APPEND _te_priv_deps TechEngine::${dep})
  endforeach()

  target_link_libraries(${_te_target}
    PUBLIC  ${_te_pub_deps} ${TM_LIBS}
    PRIVATE ${_te_priv_deps} ${TM_LIBS_PRIVATE} te_warnings)

  set_target_properties(${_te_target} PROPERTIES FOLDER "engine")

  if(TE_ENABLE_CLANG_TIDY AND TE_CLANG_TIDY_COMMAND)
    set_target_properties(${_te_target} PROPERTIES CXX_CLANG_TIDY "${TE_CLANG_TIDY_COMMAND}")
  endif()
endfunction()
