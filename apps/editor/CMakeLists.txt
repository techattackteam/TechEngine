# editor — host of app; owns the asset pipeline (import + bake).
# Composition = app + client + core (ADR-006 §1). ADR-017 decided no `tooling` tier is
# created for now, so editor-only code lives in the app itself.
techengine_app(editor
  MAIN    src/main.cpp
  SOURCES src/EditorApp.cpp
  HEADERS src/EditorApp.hpp
  DEPS    app client core
  TESTS   tests/EditorAppTests.cpp)
# Slice 2+: tooling deps (ImGui, ImGuizmo, assimp, stb) link here only.
