#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/VirtualPath.hpp>

#include <assets/TextureImporter.hpp>
#include <edit/EditSession.hpp>
#include <edit/ImportModelCommand.hpp>
#include <edit/MaterialSlotRebuild.hpp>
#include <edit/ReimportCommand.hpp>
#include <project/ProjectError.hpp>

#include <memory>
#include <span>
#include <system_error>
#include <utility>

namespace TechEngine {
    ReimportCommand::ReimportCommand(App& app, JobSystem& jobs, FileAccess& files, AssetIndex& index, EditSession& session, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, std::vector<BatchId>& batches, ReimportRequest request)
        : m_app(&app), m_jobs(&jobs), m_files(&files), m_index(&index), m_session(&session), m_resources(&resources), m_sceneHandle(&sceneHandle), m_batches(&batches), m_request(std::move(request)) {
    }

    void ReimportCommand::apply(Scene&) {
        if (m_session->getPlayState() != PlayState::Stopped) {
            const std::error_code refusal = ProjectError::EditorNotStopped;
            TE_LOGGER_ERROR("Failed to re-import {0}: {1}", m_request.sourcePath, refusal.message());
            return;
        }

        Task task = [app = m_app, files = m_files, index = m_index, session = m_session, resources = m_resources, sceneHandle = m_sceneHandle, request = std::move(m_request)] {
            std::vector<AssetRecord> records;
            std::vector<ResourceId> rewritten;
            const std::error_code reimportError = reimport(*files, request, records, rewritten);
            if (reimportError) {
                TE_LOGGER_ERROR("Failed to re-import {0}: {1}", request.sourcePath, reimportError.message());
                return;
            }
            TE_LOGGER_INFO("Re-imported {0}: {1} files kept their IDs and {2} are new", request.sourcePath, rewritten.size(), records.size());

            const std::error_code submitError = app->submitCommand(std::make_unique<ReloadImportedAssetsCommand>(*session, *resources, *sceneHandle, *index, std::move(records), std::move(rewritten)));
            if (submitError) {
                TE_LOGGER_WARN("The simulation refused the reload after re-importing {0}, so its files are indexed and loaded again on the next launch: {1}", request.sourcePath, submitError.message());
            }
        };
        m_batches->push_back(m_jobs->submit(std::span<Task>{&task, 1}));
    }

    std::error_code ReimportCommand::reimport(FileAccess& files, const ReimportRequest& request, std::vector<AssetRecord>& records, std::vector<ResourceId>& rewritten) {
        if (TextureImporter::isHdr(request.sourcePath)) {
            SkyImportRequest skyRequest;
            skyRequest.sourcePath = request.sourcePath;
            SkyImportResult skyResult;
            const TextureImporter skyImporter(files, &randomResourceId);
            const std::error_code skyError = skyImporter.reimportSky(skyRequest, skyResult);
            if (skyError) {
                return skyError;
            }
            records = std::move(skyResult.records);
            rewritten = std::move(skyResult.rewritten);
            return {};
        }

        VirtualPathParts parts;
        const std::error_code splitError = splitVirtualPath(request.sourcePath, parts);
        if (splitError) {
            return splitError;
        }
        ModelImportRequest modelRequest;
        modelRequest.sourcePath = request.sourcePath;
        modelRequest.mode = request.mode;
        if (!TextureImporter::sideOf(parts.relative, modelRequest.side)) {
            return ProjectError::SourceNotInAssets;
        }
        ModelImportResult modelResult;
        const ModelImporter modelImporter(files, &randomResourceId);
        const std::error_code modelError = modelImporter.reimportFile(modelRequest, modelResult);
        if (modelError) {
            return modelError;
        }
        records = std::move(modelResult.records);
        rewritten = std::move(modelResult.rewritten);
        return {};
    }

    ReloadImportedAssetsCommand::ReloadImportedAssetsCommand(EditSession& session, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, AssetIndex& index, std::vector<AssetRecord> records, std::vector<ResourceId> rewritten)
        : m_session(&session), m_resources(&resources), m_sceneHandle(&sceneHandle), m_index(&index), m_records(std::move(records)), m_rewritten(std::move(rewritten)) {
    }

    // The new files are indexed first, so the reload's loaders find the new materials and textures they name.
    void ReloadImportedAssetsCommand::apply(Scene& scene) {
        RegisterImportedAssetsCommand{*m_index, std::move(m_records)}.apply(scene);
        m_records.clear();
        if (m_session->getPlayState() != PlayState::Stopped) {
            const std::error_code refusal = ProjectError::EditorNotStopped;
            TE_LOGGER_ERROR("The {0} re-imported assets are not reloaded: {1}", m_rewritten.size(), refusal.message());
            return;
        }

        const std::error_code reloadError = m_resources->reload(m_rewritten);
        if (reloadError) {
            TE_LOGGER_ERROR("Failed to reload the {0} re-imported assets: {1}", m_rewritten.size(), reloadError.message());
        }
        if (m_sceneHandle->valid()) {
            rebuildMaterialSlots(scene, *m_resources, *m_sceneHandle);
        }
    }
}
