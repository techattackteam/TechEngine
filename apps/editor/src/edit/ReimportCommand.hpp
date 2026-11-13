#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImporter.hpp>

#include <string>
#include <system_error>
#include <vector>

namespace TechEngine {
    class App;
    class EditSession;
    class FileAccess;
    class ResourceCache;
    struct SceneBytes;

    struct ReimportRequest {
        // A .hdr re-imports the sky texture beside it, and the mode is not read. Any other source re-imports its folder.
        std::string sourcePath;
        ImportMode mode = ImportMode::Model;
    };

    class ReimportCommand final : public Command {
    private:
        App* m_app = nullptr;
        JobSystem* m_jobs = nullptr;
        FileAccess* m_files = nullptr;
        AssetIndex* m_index = nullptr;
        EditSession* m_session = nullptr;
        ResourceCache* m_resources = nullptr;
        const ResourceHandle<SceneBytes>* m_sceneHandle = nullptr;
        std::vector<BatchId>* m_batches = nullptr;
        ReimportRequest m_request;

    public:
        ReimportCommand(App& app, JobSystem& jobs, FileAccess& files, AssetIndex& index, EditSession& session, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, std::vector<BatchId>& batches, ReimportRequest request);

        // Refused with a log line unless the editor is Stopped. Otherwise starts the re-import as a job, adds its batch to
        // `batches` and returns at once. The job submits one ReloadImportedAssetsCommand when the files are written, so
        // everything passed here must outlive that batch.
        void apply(Scene& scene) override;

    private:
        static std::error_code reimport(FileAccess& files, const ReimportRequest& request, std::vector<AssetRecord>& records, std::vector<ResourceId>& rewritten);
    };

    class ReloadImportedAssetsCommand final : public Command {
    private:
        EditSession* m_session = nullptr;
        ResourceCache* m_resources = nullptr;
        const ResourceHandle<SceneBytes>* m_sceneHandle = nullptr;
        AssetIndex* m_index = nullptr;
        std::vector<AssetRecord> m_records;
        std::vector<ResourceId> m_rewritten;

    public:
        ReloadImportedAssetsCommand(EditSession& session, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, AssetIndex& index, std::vector<AssetRecord> records, std::vector<ResourceId> rewritten);

        // Indexes the new files, reloads the rewritten ids that the cache holds, then rebuilds the open scene's material
        // slots. Reloads nothing unless the editor is Stopped.
        void apply(Scene& scene) override;
    };
}
