techengine_module(platform
  SOURCES src/Platform.cpp
  HEADERS include/TechEngine/platform/Platform.hpp
  DEPS    base
  LIBS    glfw) # OS seam: window/input (ADR-006 §1). glad2 wired once vendored.
