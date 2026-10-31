# editor: host of app; owns the asset pipeline (import + bake).
# Composition = app + client + core (ADR-006 §1). ADR-017 decided no `tooling` tier is
# created for now, so editor-only code lives in the app itself.
techengine_app(editor
  MAIN    src/main.cpp
  SOURCES src/EditorApp.cpp
          src/assets/AssetIndex.cpp
          src/assets/LooseSource.cpp
          src/edit/EditSession.cpp
          src/edit/EditorCommands.cpp
          src/edit/EditorSnapshot.cpp
          src/edit/EditorSnapshotSlot.cpp
          src/edit/FieldCollector.cpp
          src/edit/FieldSetter.cpp
          src/edit/FieldValue.cpp
          src/edit/HierarchyCommands.cpp
          src/edit/InspectorCommands.cpp
          src/edit/SaveSceneCommand.cpp
          src/project/Project.cpp
          src/project/ProjectError.cpp
          src/ui/ClosePrompt.cpp
          src/ui/EditorTheme.cpp
          src/ui/ImGuiBridge.cpp
          src/ui/PanelsManager.cpp
          src/ui/panels/GameViewPanel.cpp
          src/ui/panels/HierarchyPanel.cpp
          src/ui/panels/InspectorPanel.cpp
          src/ui/panels/Panel.cpp
          src/ui/panels/SceneViewPanel.cpp
          src/ui/widgets/DragControls.cpp
          src/ui/widgets/PropertyTable.cpp
          src/ui/widgets/TextEditField.cpp
  HEADERS src/EditorApp.hpp
          src/assets/AssetIndex.hpp
          src/assets/LooseSource.hpp
          src/edit/EditRecord.hpp
          src/edit/EditSession.hpp
          src/edit/EditorCommands.hpp
          src/edit/EditorSnapshot.hpp
          src/edit/EditorSnapshotSlot.hpp
          src/edit/FieldCollector.hpp
          src/edit/FieldSetter.hpp
          src/edit/FieldValue.hpp
          src/edit/HierarchyCommands.hpp
          src/edit/InspectorCommands.hpp
          src/edit/SaveSceneCommand.hpp
          src/project/Project.hpp
          src/project/ProjectError.hpp
          src/ui/ClosePrompt.hpp
          src/ui/EditorTheme.hpp
          src/ui/ImGuiBridge.hpp
          src/ui/PanelsManager.hpp
          src/ui/panels/GameViewPanel.hpp
          src/ui/panels/HierarchyPanel.hpp
          src/ui/panels/InspectorPanel.hpp
          src/ui/panels/Panel.hpp
          src/ui/panels/SceneViewPanel.hpp
          src/ui/widgets/DragControls.hpp
          src/ui/widgets/PropertyTable.hpp
          src/ui/widgets/TextEditField.hpp
  DEPS    clientapp app core
  LIBS    TechEngine::imgui
  LIBS_PRIVATE TechEngine::tomlplusplus
  TESTS   tests/EditorAppTests.cpp
          tests/assets/AssetIndexTests.cpp
          tests/assets/LooseSourceTests.cpp
          tests/edit/EditSessionTests.cpp
          tests/edit/EditorCommandsTests.cpp
          tests/edit/EditorSnapshotSlotTests.cpp
          tests/edit/EditorSnapshotTests.cpp
          tests/edit/FieldValueTests.cpp
          tests/edit/HierarchyCommandsTests.cpp
          tests/edit/InspectorCommandsTests.cpp
          tests/edit/SaveSceneCommandTests.cpp
          tests/project/ProjectErrorTests.cpp
          tests/project/ProjectTests.cpp)

target_compile_definitions(editor PRIVATE
  TE_EDITOR_DEFAULT_PROJECT_ROOT="${PROJECT_SOURCE_DIR}/projects/dev")

# Slice 2+: tooling deps (ImGui, ImGuizmo, assimp, stb) link here only.
