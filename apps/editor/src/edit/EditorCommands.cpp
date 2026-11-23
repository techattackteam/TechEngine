#include <TechEngine/app/App.hpp>
#include <TechEngine/app/Command.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>

#include <assets/ModelImporter.hpp>
#include <assets/TextureImporter.hpp>
#include <edit/CreateFolderCommand.hpp>
#include <edit/CreateMaterialCommand.hpp>
#include <edit/EditorCommands.hpp>
#include <edit/HierarchyCommands.hpp>
#include <edit/ImportModelCommand.hpp>
#include <edit/ImportSkyCommand.hpp>
#include <edit/InspectorCommands.hpp>
#include <edit/PlaceModelCommand.hpp>
#include <edit/RefreshAssetsCommand.hpp>
#include <edit/SaveSceneCommand.hpp>

#include <system_error>
#include <utility>

namespace TechEngine {
    EditorCommands::EditorCommands(App& app, EditSession& session, const ComponentRegistry& registry, FileAccess& files, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, AssetIndex& assets, JobSystem& jobs, std::vector<BatchId>& importBatches)
        : m_app(&app), m_session(&session), m_registry(&registry), m_files(&files), m_resources(&resources), m_sceneHandle(&sceneHandle), m_assets(&assets), m_jobs(&jobs), m_importBatches(&importBatches) {
    }

    void EditorCommands::create(const Entity parent) {
        submit(std::make_unique<CreateEntityCommand>(*m_session, parent));
    }

    void EditorCommands::destroy(std::vector<Entity> entities) {
        submit(std::make_unique<DestroyEntityCommand>(*m_session, std::move(entities)));
    }

    void EditorCommands::reparent(std::vector<Entity> children, const Entity parent, const std::size_t position) {
        submit(std::make_unique<ReparentEntityCommand>(*m_session, std::move(children), parent, position));
    }

    void EditorCommands::rename(std::vector<Entity> entities, std::string name) {
        submit(std::make_unique<RenameEntityCommand>(*m_session, std::move(entities), std::move(name)));
    }

    void EditorCommands::select(std::vector<Entity> selection, const Entity active, const std::uint64_t sequence) {
        submit(std::make_unique<SelectEntityCommand>(*m_session, std::move(selection), active, sequence));
    }

    void EditorCommands::setField(std::vector<Entity> entities, const ComponentTypeId component, const std::uint32_t field, FieldValue value) {
        submit(std::make_unique<SetFieldCommand>(*m_session, *m_registry, std::move(entities), component, field, std::move(value)));
    }

    void EditorCommands::setAssetField(std::vector<Entity> entities, const ComponentTypeId component, const std::uint32_t field, const ResourceId id) {
        submit(std::make_unique<SetAssetFieldCommand>(*m_session, *m_registry, *m_resources, *m_sceneHandle, *m_assets, std::move(entities), component, field, id));
    }

    void EditorCommands::setRotationAxis(std::vector<Entity> entities, const Entity active, const Quat activeRotation, const std::uint32_t axis, const float degrees) {
        submit(std::make_unique<SetRotationAxisCommand>(*m_session, *m_registry, std::move(entities), active, activeRotation, axis, degrees));
    }

    void EditorCommands::setWorldAxis(std::vector<Entity> entities, const Entity active, const Quat activeRotation, const TransformPart part, const std::uint32_t axis, const float value) {
        submit(std::make_unique<SetWorldAxisCommand>(*m_session, std::move(entities), active, activeRotation, part, axis, value));
    }

    void EditorCommands::addComponent(std::vector<Entity> entities, const ComponentTypeId component) {
        submit(std::make_unique<AddComponentCommand>(*m_session, std::move(entities), component));
    }

    void EditorCommands::removeComponent(std::vector<Entity> entities, const ComponentTypeId component) {
        submit(std::make_unique<RemoveComponentCommand>(*m_session, *m_registry, std::move(entities), component));
    }

    void EditorCommands::placeModel(std::string path) {
        submit(std::make_unique<PlaceModelCommand>(*m_session, *m_files, *m_resources, *m_sceneHandle, *m_assets, std::move(path)));
    }

    void EditorCommands::refreshAssets() {
        submit(std::make_unique<RefreshAssetsCommand>(*m_files, *m_assets));
    }

    void EditorCommands::createModel(ModelImportRequest request) {
        submit(std::make_unique<ImportModelCommand>(*m_app, *m_jobs, *m_files, *m_assets, *m_importBatches, std::move(request)));
    }

    void EditorCommands::createSky(SkyImportRequest request) {
        submit(std::make_unique<ImportSkyCommand>(*m_app, *m_jobs, *m_files, *m_assets, *m_importBatches, std::move(request)));
    }

    void EditorCommands::createMaterial(MaterialCreateRequest request) {
        submit(std::make_unique<CreateMaterialCommand>(*m_files, *m_assets, std::move(request)));
    }

    void EditorCommands::createFolder(FolderCreateRequest request) {
        submit(std::make_unique<CreateFolderCommand>(*m_files, *m_assets, std::move(request)));
    }

    void EditorCommands::save() {
        submit(std::make_unique<SaveSceneCommand>(*m_session, *m_files, *m_app, AfterSave::KeepOpen));
    }

    void EditorCommands::saveAndClose() {
        submit(std::make_unique<SaveSceneCommand>(*m_session, *m_files, *m_app, AfterSave::Close));
    }

    void EditorCommands::submit(std::unique_ptr<Command> command) {
        const std::error_code submitError = m_app->submitCommand(std::move(command));
        if (submitError) {
            TE_LOGGER_WARN("The simulation refused an edit: {0}", submitError.message());
        }
    }
}
