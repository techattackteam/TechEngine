techengine_module(app
  SOURCES src/App.cpp src/FrameLoop.cpp src/diagnostics/Diagnostics.cpp src/diagnostics/MemoryTracking.cpp
  HEADERS include/TechEngine/app/App.hpp include/TechEngine/app/FrameLoop.hpp include/TechEngine/app/EntryPoint.hpp
          src/diagnostics/Diagnostics.hpp src/diagnostics/MemoryTracking.hpp
  DEPS    core base
  DEPS_PRIVATE platform)

# TODO(S3-T13): throwaway with the demo mount in App.cpp. Bakes a source-tree path into the
# binary, which only holds because the demo dies at M3 — the real mount set comes from the
# loaded project, and an installed build would need the exe's own directory instead.
target_compile_definitions(TechEngineApp PRIVATE TE_DEMO_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
# app = game loop + composition root, presentation-agnostic (ADR-006 §1/§4).
# core and base are both PUBLIC — FrameContext and the profiler macros sit in FrameLoop.hpp;
# the exes add client where presentation is wanted.

if(TE_BUILD_TESTS)
  techengine_test(app SOURCES tests/AppTests.cpp tests/FrameLoopTests.cpp tests/diagnostics/DiagnosticsTests.cpp)
endif()
