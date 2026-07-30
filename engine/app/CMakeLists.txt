techengine_module(app
  SOURCES src/App.cpp src/FrameLoop.cpp
  HEADERS include/TechEngine/app/App.hpp include/TechEngine/app/FrameLoop.hpp
  DEPS    core
  DEPS_PRIVATE base)
# app = game loop + composition root, presentation-agnostic (ADR-006 §1/§4).
# Depends on core ONLY; the exes add client where presentation is wanted.
