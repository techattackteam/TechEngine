# runtime — shipped standalone: a client, and a listen-server when it also runs
# authoritative sim. Composition = app + client + core (ADR-006 §1).
add_executable(runtime src/main.cpp)

target_link_libraries(runtime PRIVATE
  TechEngine::app
  TechEngine::client
  TechEngine::core
  te_warnings)

set_target_properties(runtime PROPERTIES FOLDER "apps")
