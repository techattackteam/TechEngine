techengine_module(app
  SOURCES src/App.cpp src/SimulationThread.cpp src/diagnostics/Diagnostics.cpp src/diagnostics/MemoryTracking.cpp
  HEADERS include/TechEngine/app/App.hpp include/TechEngine/app/EntryPoint.hpp include/TechEngine/app/SimulationThread.hpp
          src/diagnostics/Diagnostics.hpp src/diagnostics/MemoryTracking.hpp
  DEPS    core base platform)

# app = game loop + composition root, presentation-agnostic (ADR-006 §1/§4).
# core, base and platform are PUBLIC — SimulationContext and the profiler macros sit in
# SimulationThread.hpp, and App.hpp owns the input buffer; the exes add client where
# presentation is wanted.

if(TE_BUILD_TESTS)
  techengine_test(app SOURCES tests/AppTests.cpp tests/SimulationThreadTests.cpp tests/diagnostics/DiagnosticsTests.cpp)
endif()
