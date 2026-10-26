# editor: host of app; owns the asset pipeline (import + bake).
# Composition = app + client + core (ADR-006 §1). ADR-017 decided no `tooling` tier is
# created for now, so editor-only code lives in the app itself.
techengine_app(editor
  MAIN    src/main.cpp
  SOURCES src/EditorApp.cpp
          src/assets/AssetIndex.cpp
          src/assets/LooseSource.cpp
          src/project/Project.cpp
          src/project/ProjectError.cpp
          src/ui/ImGuiBridge.cpp
  HEADERS src/EditorApp.hpp
          src/assets/AssetIndex.hpp
          src/assets/LooseSource.hpp
          src/project/Project.hpp
          src/project/ProjectError.hpp
          src/ui/ImGuiBridge.hpp
  DEPS    clientapp app core
  LIBS    TechEngine::imgui
  LIBS_PRIVATE TechEngine::tomlplusplus
  TESTS   tests/EditorAppTests.cpp
          tests/assets/AssetIndexTests.cpp
          tests/assets/LooseSourceTests.cpp
          tests/project/ProjectErrorTests.cpp
          tests/project/ProjectTests.cpp)

target_compile_definitions(editor PRIVATE
  TE_EDITOR_DEFAULT_PROJECT_ROOT="${PROJECT_SOURCE_DIR}/projects/dev")

# Slice 2+: tooling deps (ImGui, ImGuizmo, assimp, stb) link here only.
