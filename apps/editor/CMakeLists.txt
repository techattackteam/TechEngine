# editor — host of app; owns the asset pipeline (import + bake).
# Composition = app + client + core (ADR-006 §1). ADR-017 decided no `tooling` tier is
# created for now, so editor-only code lives in the app itself.
techengine_app(editor
  MAIN    src/main.cpp
  SOURCES src/EditorApp.cpp
          src/project/Project.cpp
  HEADERS src/EditorApp.hpp
          src/project/Project.hpp
  DEPS    app client core
  LIBS    TechEngine::tomlplusplus
  TESTS   tests/EditorAppTests.cpp
          tests/project/ProjectTests.cpp)
# Slice 2+: tooling deps (ImGui, ImGuizmo, assimp, stb) link here only.
