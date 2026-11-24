# editor: host of app; owns the asset pipeline (import + bake).
# Composition = app + client + core (ADR-006 §1). ADR-017 decided no `tooling` tier is
# created for now, so editor-only code lives in the app itself.
techengine_app(editor
  MAIN    src/main.cpp
  SOURCES src/EditorApp.cpp
          src/assets/import/ImageDecoder.cpp
          src/assets/import/MaterialImporter.cpp
          src/assets/import/MeshImporter.cpp
          src/assets/import/ModelImporter.cpp
          src/assets/import/TextureImporter.cpp
          src/assets/index/AssetIndex.cpp
          src/assets/index/AssetNaming.cpp
          src/assets/index/LooseSource.cpp
          src/edit/EditorCommands.cpp
          src/edit/assets/CreateFolderCommand.cpp
          src/edit/assets/CreateMaterialCommand.cpp
          src/edit/assets/ImportModelCommand.cpp
          src/edit/assets/ImportSkyCommand.cpp
          src/edit/assets/RefreshAssetsCommand.cpp
          src/edit/camera/FlyCamera.cpp
          src/edit/console/ConsoleHistory.cpp
          src/edit/console/ConsoleLog.cpp
          src/edit/fields/FieldCollector.cpp
          src/edit/fields/FieldSetter.cpp
          src/edit/fields/FieldValue.cpp
          src/edit/project/ExportCommand.cpp
          src/edit/project/SaveSceneCommand.cpp
          src/edit/scene/HierarchyCommands.cpp
          src/edit/scene/InspectorCommands.cpp
          src/edit/scene/MaterialSlotRebuild.cpp
          src/edit/scene/PlaceModelCommand.cpp
          src/edit/session/EditSession.cpp
          src/edit/snapshot/EditorSnapshot.cpp
          src/edit/snapshot/EditorSnapshotSlot.cpp
          src/project/Project.cpp
          src/project/ProjectError.cpp
          src/ui/ClosePrompt.cpp
          src/ui/EditorTheme.cpp
          src/ui/ImGuiBridge.cpp
          src/ui/PanelsManager.cpp
          src/ui/ViewTextureSwap.cpp
          src/ui/creators/AssetCreators.cpp
          src/ui/panels/AssetBrowserPanel.cpp
          src/ui/panels/ConsolePanel.cpp
          src/ui/panels/GameViewPanel.cpp
          src/ui/panels/HierarchyPanel.cpp
          src/ui/panels/InspectorPanel.cpp
          src/ui/panels/Panel.cpp
          src/ui/panels/SceneViewPanel.cpp
          src/ui/panels/ViewPanel.cpp
          src/ui/widgets/DragControls.cpp
          src/ui/widgets/PropertyTable.cpp
          src/ui/widgets/TextEditField.cpp
  HEADERS src/EditorApp.hpp
          src/assets/import/ImageDecoder.hpp
          src/assets/import/MaterialImporter.hpp
          src/assets/import/MeshImporter.hpp
          src/assets/import/ModelImporter.hpp
          src/assets/import/TextureImporter.hpp
          src/assets/index/AssetIndex.hpp
          src/assets/index/AssetNaming.hpp
          src/assets/index/LooseSource.hpp
          src/edit/EditorCommands.hpp
          src/edit/assets/CreateFolderCommand.hpp
          src/edit/assets/CreateMaterialCommand.hpp
          src/edit/assets/ImportModelCommand.hpp
          src/edit/assets/ImportSkyCommand.hpp
          src/edit/assets/RefreshAssetsCommand.hpp
          src/edit/camera/FlyCamera.hpp
          src/edit/console/ConsoleHistory.hpp
          src/edit/console/ConsoleLog.hpp
          src/edit/fields/FieldCollector.hpp
          src/edit/fields/FieldSetter.hpp
          src/edit/fields/FieldValue.hpp
          src/edit/project/ExportCommand.hpp
          src/edit/project/SaveSceneCommand.hpp
          src/edit/scene/HierarchyCommands.hpp
          src/edit/scene/InspectorCommands.hpp
          src/edit/scene/MaterialSlotRebuild.hpp
          src/edit/scene/PlaceModelCommand.hpp
          src/edit/session/EditRecord.hpp
          src/edit/session/EditSession.hpp
          src/edit/snapshot/EditorSnapshot.hpp
          src/edit/snapshot/EditorSnapshotSlot.hpp
          src/project/Project.hpp
          src/project/ProjectError.hpp
          src/ui/ClosePrompt.hpp
          src/ui/EditorTheme.hpp
          src/ui/ImGuiBridge.hpp
          src/ui/PanelsManager.hpp
          src/ui/ViewTextureSwap.hpp
          src/ui/creators/AssetCreator.hpp
          src/ui/creators/AssetCreators.hpp
          src/ui/panels/AssetBrowserPanel.hpp
          src/ui/panels/ConsolePanel.hpp
          src/ui/panels/GameViewPanel.hpp
          src/ui/panels/HierarchyPanel.hpp
          src/ui/panels/InspectorPanel.hpp
          src/ui/panels/Panel.hpp
          src/ui/panels/SceneViewPanel.hpp
          src/ui/panels/ViewPanel.hpp
          src/ui/widgets/DragControls.hpp
          src/ui/widgets/PropertyTable.hpp
          src/ui/widgets/TextEditField.hpp
  DEPS    clientapp app core
  LIBS    TechEngine::imgui assimp::assimp TechEngine::mikktspace meshoptimizer TechEngine::stb TechEngine::bcdec TechEngine::bc7enc
  LIBS_PRIVATE TechEngine::tomlplusplus
  TESTS   tests/EditorAppTests.cpp
          tests/assets/import/AssimpTests.cpp
          tests/assets/import/ImageDecoderTests.cpp
          tests/assets/import/MaterialImportTests.cpp
          tests/assets/import/MergedMeshImportTests.cpp
          tests/assets/import/ModelImporterTests.cpp
          tests/assets/import/SkyImportTests.cpp
          tests/assets/import/TextureImporterTests.cpp
          tests/assets/index/AssetIndexTests.cpp
          tests/assets/index/AssetNamingTests.cpp
          tests/assets/index/LooseSourceTests.cpp
          tests/edit/EditorCommandsTests.cpp
          tests/edit/assets/CreateFolderCommandTests.cpp
          tests/edit/assets/CreateMaterialCommandTests.cpp
          tests/edit/assets/ImportModelCommandTests.cpp
          tests/edit/assets/ImportSkyCommandTests.cpp
          tests/edit/assets/RefreshAssetsCommandTests.cpp
          tests/edit/camera/FlyCameraTests.cpp
          tests/edit/console/ConsoleHistoryTests.cpp
          tests/edit/console/ConsoleLogTests.cpp
          tests/edit/fields/FieldValueTests.cpp
          tests/edit/project/SaveSceneCommandTests.cpp
          tests/edit/scene/HierarchyCommandsTests.cpp
          tests/edit/scene/InspectorCommandsTests.cpp
          tests/edit/scene/MainCameraCommandsTests.cpp
          tests/edit/scene/MaterialSlotRebuildTests.cpp
          tests/edit/scene/PlaceModelCommandTests.cpp
          tests/edit/session/EditSessionTests.cpp
          tests/edit/snapshot/EditorSnapshotSlotTests.cpp
          tests/edit/snapshot/EditorSnapshotTests.cpp
          tests/project/ProjectErrorTests.cpp
          tests/project/ProjectTests.cpp
          tests/ui/AssetBrowserPanelTests.cpp
          tests/ui/SceneViewPanelTests.cpp
          tests/ui/ViewTextureSwapTests.cpp)

# On the object library, so the editor's tests can export the dev project too.
target_compile_definitions(te_editor_objects PUBLIC
  TE_EDITOR_DEFAULT_PROJECT_ROOT="${PROJECT_SOURCE_DIR}/projects/dev")
target_compile_definitions(editor PRIVATE
  TE_EDITOR_RUNTIME_FILE_NAME="$<TARGET_FILE_NAME:runtime>")

# File -> Export copies the runtime from the editor's folder.
add_dependencies(editor runtime)

# Slice 2+: tooling deps (ImGui, ImGuizmo, assimp, stb) link here only.
