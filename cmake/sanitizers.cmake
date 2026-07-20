# Preset-driven sanitizers (ADR-008 §3/§5).
#
# A configure preset sets TE_SANITIZER=asan|ubsan|tsan; the flags are added to
# te_warnings so they reach our targets only (never FetchContent deps). Sanitizer
# choice per platform is ADR-005: ASan on Win/MSVC, UBSan + TSan on Linux/Clang.
#
# Must be included AFTER warnings.cmake (it mutates te_warnings).

if(NOT TARGET te_warnings)
  message(FATAL_ERROR "sanitizers.cmake requires te_warnings (include warnings.cmake first)")
endif()

set(TE_SANITIZER "" CACHE STRING "Sanitizer to enable: '' | asan | ubsan | tsan")
set_property(CACHE TE_SANITIZER PROPERTY STRINGS "" asan ubsan tsan)

if(TE_SANITIZER STREQUAL "")
  return()
endif()

if(MSVC)
  if(NOT TE_SANITIZER STREQUAL "asan")
    message(FATAL_ERROR "MSVC supports only TE_SANITIZER=asan (got '${TE_SANITIZER}'); "
                        "UBSan/TSan live on the Linux/Clang leg (ADR-005).")
  endif()
  target_compile_options(te_warnings INTERFACE /fsanitize=address)
  target_compile_definitions(te_warnings INTERFACE
    _DISABLE_STRING_ANNOTATION=1
    _DISABLE_VECTOR_ANNOTATION=1)
else()
  if(TE_SANITIZER STREQUAL "asan")
    set(_te_san address)
  elseif(TE_SANITIZER STREQUAL "ubsan")
    set(_te_san undefined)
  elseif(TE_SANITIZER STREQUAL "tsan")
    set(_te_san thread)
  else()
    message(FATAL_ERROR "Unknown TE_SANITIZER '${TE_SANITIZER}' (expected asan|ubsan|tsan)")
  endif()
  target_compile_options(te_warnings INTERFACE
    -fsanitize=${_te_san} -fno-omit-frame-pointer -g)
  target_link_options(te_warnings INTERFACE -fsanitize=${_te_san})
endif()

message(STATUS "TechEngine: sanitizer '${TE_SANITIZER}' enabled on first-party targets")
