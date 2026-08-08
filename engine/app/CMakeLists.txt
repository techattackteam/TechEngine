techengine_module(app
  SOURCES src/App.cpp src/FrameLoop.cpp src/diagnostics/Diagnostics.cpp src/diagnostics/MemoryTracking.cpp
  HEADERS include/TechEngine/app/App.hpp include/TechEngine/app/FrameLoop.hpp
          src/diagnostics/Diagnostics.hpp src/diagnostics/MemoryTracking.hpp
  DEPS    core base)
# app = game loop + composition root, presentation-agnostic (ADR-006 §1/§4).
# core and base are both PUBLIC — FrameContext and the profiler macros sit in FrameLoop.hpp;
# the exes add client where presentation is wanted.

if(TE_BUILD_TESTS)
  techengine_test(app SOURCES tests/FrameLoopTests.cpp tests/diagnostics/DiagnosticsTests.cpp)
endif()
