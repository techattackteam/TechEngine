techengine_module(app
  SOURCES src/App.cpp src/FrameLoop.cpp src/diagnostics/MemoryTracking.cpp
  HEADERS include/TechEngine/app/App.hpp include/TechEngine/app/FrameLoop.hpp
          src/diagnostics/MemoryTracking.hpp
  DEPS    core
  DEPS_PRIVATE base)
# app = game loop + composition root, presentation-agnostic (ADR-006 §1/§4).
# core is PUBLIC (FrameContext sits in its headers), base PRIVATE (Clock + the frame-stamp
# push are App.cpp-only); the exes add client where presentation is wanted.

if(TE_BUILD_TESTS)
  techengine_test(app SOURCES tests/FrameLoopTests.cpp)
endif()
