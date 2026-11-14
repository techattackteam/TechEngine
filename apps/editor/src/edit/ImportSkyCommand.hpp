#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/TextureImporter.hpp>

#include <vector>

namespace TechEngine {
    class App;
    class FileAccess;

    class ImportSkyCommand final : public Command {
    private:
        App* m_app = nullptr;
        JobSystem* m_jobs = nullptr;
        FileAccess* m_files = nullptr;
        AssetIndex* m_index = nullptr;
        std::vector<BatchId>* m_batches = nullptr;
        SkyImportRequest m_request;

    public:
        ImportSkyCommand(App& app, JobSystem& jobs, FileAccess& files, AssetIndex& index, std::vector<BatchId>& batches, SkyImportRequest request);

        // Starts the import as a job, adds its batch to `batches` and returns at once. The job touches the index only
        // through a RegisterImportedAssetsCommand it submits when the file is written, so everything passed here must
        // outlive that batch.
        void apply(Scene& scene) override;
    };
}
