# editor: host of app; owns the asset pipeline (import + bake).
# Composition = app + client + core (ADR-006 §1). ADR-017 decided no `tooling` tier is
# created for now, so editor-only code lives in the app itself.
techengine_app(editor
  MAIN    src/main.cpp
  SOURCES src/EditorApp.cpp
          src/assets/AssetIndex.cpp
          src/assets/Bcdec.cpp
          src/assets/DdsReader.cpp
          src/assets/ImageDecoder.cpp
          src/assets/ImageImporter.cpp
          src/assets/ImportClaim.cpp
          src/assets/ImportedFile.cpp
          src/assets/LooseSource.cpp
          src/assets/MaterialImporter.cpp
          src/assets/MeshImporter.cpp
          src/assets/MipBuilder.cpp
          src/assets/ModelImporter.cpp
          src/assets/ReimportMatcher.cpp
          src/assets/Rgb9E5.cpp
          src/assets/StbImage.cpp
          src/assets/TextureImporter.cpp
          src/edit/ConsoleHistory.cpp
          src/edit/ConsoleLog.cpp
          src/edit/EditSession.cpp
          src/edit/EditorCommands.cpp
          src/edit/EditorSnapshot.cpp
          src/edit/EditorSnapshotSlot.cpp
          src/edit/ExportCommand.cpp
          src/edit/FieldCollector.cpp
          src/edit/FieldSetter.cpp
          src/edit/FieldValue.cpp
          src/edit/HierarchyCommands.cpp
          src/edit/ImportImageCommand.cpp
          src/edit/ImportModelCommand.cpp
          src/edit/InspectorCommands.cpp
          src/edit/MaterialSlotRebuild.cpp
          src/edit/PlaceModelCommand.cpp
          src/edit/ReimportCommand.cpp
          src/edit/SaveSceneCommand.cpp
          src/project/Project.cpp
          src/project/ProjectError.cpp
          src/ui/ClosePrompt.cpp
          src/ui/EditorTheme.cpp
          src/ui/ImGuiBridge.cpp
          src/ui/PanelsManager.cpp
          src/ui/panels/ConsolePanel.cpp
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
          src/assets/DdsReader.hpp
          src/assets/ImageDecoder.hpp
          src/assets/ImageImporter.hpp
          src/assets/ImportClaim.hpp
          src/assets/ImportedFile.hpp
          src/assets/LooseSource.hpp
          src/assets/MaterialImporter.hpp
          src/assets/MeshImporter.hpp
          src/assets/MipBuilder.hpp
          src/assets/ModelImporter.hpp
          src/assets/ReimportMatcher.hpp
          src/assets/Rgb9E5.hpp
          src/assets/TextureImporter.hpp
          src/edit/ConsoleHistory.hpp
          src/edit/ConsoleLog.hpp
          src/edit/EditRecord.hpp
          src/edit/EditSession.hpp
          src/edit/EditorCommands.hpp
          src/edit/EditorSnapshot.hpp
          src/edit/EditorSnapshotSlot.hpp
          src/edit/ExportCommand.hpp
          src/edit/FieldCollector.hpp
          src/edit/FieldSetter.hpp
          src/edit/FieldValue.hpp
          src/edit/HierarchyCommands.hpp
          src/edit/ImportImageCommand.hpp
          src/edit/ImportModelCommand.hpp
          src/edit/InspectorCommands.hpp
          src/edit/MaterialSlotRebuild.hpp
          src/edit/PlaceModelCommand.hpp
          src/edit/ReimportCommand.hpp
          src/edit/SaveSceneCommand.hpp
          src/project/Project.hpp
          src/project/ProjectError.hpp
          src/ui/ClosePrompt.hpp
          src/ui/EditorTheme.hpp
          src/ui/ImGuiBridge.hpp
          src/ui/PanelsManager.hpp
          src/ui/panels/ConsolePanel.hpp
          src/ui/panels/GameViewPanel.hpp
          src/ui/panels/HierarchyPanel.hpp
          src/ui/panels/InspectorPanel.hpp
          src/ui/panels/Panel.hpp
          src/ui/panels/SceneViewPanel.hpp
          src/ui/widgets/DragControls.hpp
          src/ui/widgets/PropertyTable.hpp
          src/ui/widgets/TextEditField.hpp
  DEPS    clientapp app core
  LIBS    TechEngine::imgui assimp::assimp TechEngine::mikktspace meshoptimizer TechEngine::stb TechEngine::bcdec TechEngine::bc7enc
  LIBS_PRIVATE TechEngine::tomlplusplus
  TESTS   tests/EditorAppTests.cpp
          tests/assets/AssetIndexTests.cpp
          tests/assets/AssimpTests.cpp
          tests/assets/DdsReaderTests.cpp
          tests/assets/ImageDecoderTests.cpp
          tests/assets/ImageImporterTests.cpp
          tests/assets/ImportClaimTests.cpp
          tests/assets/LooseSourceTests.cpp
          tests/assets/MaterialImportTests.cpp
          tests/assets/MergedMeshImportTests.cpp
          tests/assets/MipBuilderTests.cpp
          tests/assets/ModelImporterTests.cpp
          tests/assets/ModelReimportTests.cpp
          tests/assets/ReimportMatcherTests.cpp
          tests/assets/Rgb9E5Tests.cpp
          tests/assets/TextureImporterTests.cpp
          tests/edit/ConsoleHistoryTests.cpp
          tests/edit/ConsoleLogTests.cpp
          tests/edit/EditSessionTests.cpp
          tests/edit/EditorCommandsTests.cpp
          tests/edit/EditorSnapshotSlotTests.cpp
          tests/edit/EditorSnapshotTests.cpp
          tests/edit/FieldValueTests.cpp
          tests/edit/HierarchyCommandsTests.cpp
          tests/edit/ImportImageCommandTests.cpp
          tests/edit/ImportModelCommandTests.cpp
          tests/edit/InspectorCommandsTests.cpp
          tests/edit/MaterialSlotRebuildTests.cpp
          tests/edit/PlaceModelCommandTests.cpp
          tests/edit/ReimportCommandTests.cpp
          tests/edit/SaveSceneCommandTests.cpp
          tests/project/ProjectErrorTests.cpp
          tests/project/ProjectTests.cpp)

# On the object library, so the editor's tests can export the dev project too.
target_compile_definitions(te_editor_objects PUBLIC
  TE_EDITOR_DEFAULT_PROJECT_ROOT="${PROJECT_SOURCE_DIR}/projects/dev")
target_compile_definitions(editor PRIVATE
  TE_EDITOR_RUNTIME_FILE_NAME="$<TARGET_FILE_NAME:runtime>")

# File -> Export copies the runtime from the editor's folder.
add_dependencies(editor runtime)

# Slice 2+: tooling deps (ImGui, ImGuizmo, assimp, stb) link here only.
