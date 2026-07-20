techengine_module(app
  SOURCES src/App.cpp
  HEADERS include/TechEngine/app/App.hpp
  DEPS    core)
# app = game loop + composition root, presentation-agnostic (ADR-006 §1/§4).
# Depends on core ONLY; the exes add client where presentation is wanted.
