function(techengine_app name)
  cmake_parse_arguments(TA "" ""
    "MAIN;SOURCES;HEADERS;DEPS;LIBS;TESTS" ${ARGN})

  if(NOT TA_MAIN)
    message(FATAL_ERROR "techengine_app(${name}): MAIN is required (the .cpp holding main()).")
  endif()

  if(NOT TA_SOURCES)
    message(FATAL_ERROR "techengine_app(${name}): SOURCES is required (explicit list, no GLOB).")
  endif()

  # PascalCase the app dir name for the test target: runtime -> TechEngineRuntimeTests,
  # matching techengine_test()'s spelling for modules.
  string(SUBSTRING "${name}" 0 1 _te_first)
  string(TOUPPER "${_te_first}" _te_first)
  string(SUBSTRING "${name}" 1 -1 _te_rest)
  set(_te_pascal "${_te_first}${_te_rest}")

  set(_te_objects "te_${name}_objects")

  add_library(${_te_objects} OBJECT ${TA_SOURCES} ${TA_HEADERS})

  # src/ is PUBLIC here, unlike techengine_module() where it is PRIVATE. An app has no
  # include/ and no consumer outside itself: the only things that see this are its own exe
  # and its own test suite, and both need the headers beside the sources.
  target_include_directories(${_te_objects} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

  target_compile_features(${_te_objects} PUBLIC cxx_std_20)

  set(_te_deps "")
  foreach(dep IN LISTS TA_DEPS)
    list(APPEND _te_deps TechEngine::${dep})
  endforeach()

  target_link_libraries(${_te_objects}
    PUBLIC  ${_te_deps} ${TA_LIBS}
    PRIVATE te_warnings)

  set_target_properties(${_te_objects} PROPERTIES FOLDER "apps")

  if(TE_ENABLE_CLANG_TIDY AND TE_CLANG_TIDY_COMMAND)
    set_target_properties(${_te_objects} PROPERTIES CXX_CLANG_TIDY "${TE_CLANG_TIDY_COMMAND}")
  endif()

  # Linking the object library (rather than splicing $<TARGET_OBJECTS:> in by hand) is what
  # carries its include dirs and its DEPS through to the consumer as well as its objects.
  add_executable(${name} ${TA_MAIN})
  target_link_libraries(${name} PRIVATE ${_te_objects} te_warnings)
  set_target_properties(${name} PROPERTIES FOLDER "apps")

  if(NOT TE_BUILD_TESTS OR NOT TA_TESTS)
    return()
  endif()

  set(_te_test_target "TechEngine${_te_pascal}Tests")

  add_executable(${_te_test_target} ${TA_TESTS})

  target_include_directories(${_te_test_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  target_link_libraries(${_te_test_target} PRIVATE
    ${_te_objects}
    Catch2::Catch2WithMain
    te_test_support
    te_warnings)

  set_target_properties(${_te_test_target} PROPERTIES FOLDER "tests")

  # coverage.cmake reads this to build llvm-cov's -object list. Appending here is what makes
  # apps/ report coverage at all: coverage_report.cmake already lists it as a root, but no
  # instrumented object covering it ever reached the report before.
  set_property(GLOBAL APPEND PROPERTY TE_TEST_TARGETS ${_te_test_target})

  catch_discover_tests(${_te_test_target})
endfunction()
