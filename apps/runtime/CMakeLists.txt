# runtime — shipped standalone: a client, and a listen-server when it also runs
# authoritative sim. Composition = app + client + core (ADR-006 §1).
techengine_app(runtime
  MAIN    src/main.cpp
  SOURCES src/RuntimeApp.cpp
          src/demo/CollisionSystem.cpp
          src/demo/EntitySpawnSystem.cpp
          src/demo/GravitySystem.cpp
          src/demo/MovementSystem.cpp
  HEADERS src/RuntimeApp.hpp
          src/demo/CollisionSystem.hpp
          src/demo/EntitySpawnSystem.hpp
          src/demo/GravitySystem.hpp
          src/demo/MovementSystem.hpp
          src/demo/RigidBody.hpp
          src/demo/Velocity.hpp
  DEPS    app client core
  TESTS   tests/RuntimeAppTests.cpp)
