# editor: host of app; owns the asset pipeline (import + bake).
# Composition = app + client + core (ADR-006 §1). ADR-017 decided no `tooling` tier is
# created for now, so editor-only code lives in the app itself.
techengine_app(editor
  MAIN    src/main.cpp
  SOURCES src/EditorApp.cpp
          src/project/Project.cpp
          src/project/ProjectError.cpp
  HEADERS src/EditorApp.hpp
          src/project/Project.hpp
          src/project/ProjectError.hpp
  DEPS    app client core
  LIBS_PRIVATE TechEngine::tomlplusplus
  TESTS   tests/EditorAppTests.cpp
          tests/project/ProjectErrorTests.cpp
          tests/project/ProjectTests.cpp)

target_compile_definitions(editor PRIVATE
  TE_EDITOR_DEFAULT_PROJECT_ROOT="${PROJECT_SOURCE_DIR}/projects/dev")

# Slice 2+: tooling deps (ImGui, ImGuizmo, assimp, stb) link here only.
