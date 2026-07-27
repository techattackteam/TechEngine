# te_warnings — INTERFACE target carrying our warning policy (ADR-008 §5).
#
# Linked PRIVATELY by every first-party target (via techengine_module() and the
# leaf exes), never by third-party FetchContent code — so /WX (-Werror) is held
# against code we own and never breaks a vendored dep on a warning we don't.
#
# sanitizers.cmake augments THIS target after it is defined, so the sanitizer
# flags ride the same "our targets only" channel.

if(TARGET te_warnings)
  return()
endif()

add_library(te_warnings INTERFACE)
add_library(TechEngine::warnings ALIAS te_warnings)

option(TE_WERROR "Treat warnings as errors on first-party targets" ON)

if(MSVC)
  # /Zc:preprocessor — conforming preprocessor, needed for __VA_OPT__. The traditional one
  # silently drops the trailing comma of an empty __VA_ARGS__, so a macro that is fine here
  # is a hard error on the Clang leg. First-party only: vendored deps keep the default.
  target_compile_options(te_warnings INTERFACE
    /W4
    /Zc:preprocessor
    $<$<BOOL:${TE_WERROR}>:/WX>)
else()
  target_compile_options(te_warnings INTERFACE
    -Wall -Wextra -Wpedantic
    $<$<BOOL:${TE_WERROR}>:-Werror>)
endif()
