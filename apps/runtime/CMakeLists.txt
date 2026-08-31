# runtime — shipped standalone: a client, and a listen-server when it also runs
# authoritative sim. Composition = app + client + core (ADR-006 §1).
techengine_app(runtime
  MAIN    src/main.cpp
  SOURCES src/RuntimeApp.cpp
  HEADERS src/RuntimeApp.hpp
  DEPS    app client core
  TESTS   tests/RuntimeAppTests.cpp)
