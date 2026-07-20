# editor — host/observer of app, out of the frame loop; owns the asset pipeline
# (import + bake). Composition = app + client + core + tooling (ADR-006 §1).
add_executable(editor src/main.cpp)

target_link_libraries(editor PRIVATE
  TechEngine::app
  TechEngine::client
  TechEngine::core
  te_warnings)
# Slice 2+: tooling deps (ImGui, ImGuizmo, assimp, stb) link here only.

set_target_properties(editor PROPERTIES FOLDER "apps")
