#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImporter.hpp>

#include <vector>

namespace TechEngine {
    class App;
    class FileAccess;

    class ImportModelCommand final : public Command {
    private:
        App* m_app = nullptr;
        JobSystem* m_jobs = nullptr;
        FileAccess* m_files = nullptr;
        AssetIndex* m_index = nullptr;
        std::vector<BatchId>* m_batches = nullptr;
        ModelImportRequest m_request;

    public:
        ImportModelCommand(App& app, JobSystem& jobs, FileAccess& files, AssetIndex& index, std::vector<BatchId>& batches, ModelImportRequest request);

        // Starts the import as a job, adds its batch to `batches` and returns at once. The job touches the index only
        // through a second command it submits when the files are written, so everything passed here must outlive that batch.
        void apply(Scene& scene) override;
    };

    class RegisterImportedAssetsCommand final : public Command {
    private:
        AssetIndex* m_index = nullptr;
        std::vector<AssetRecord> m_records;

    public:
        RegisterImportedAssetsCommand(AssetIndex& index, std::vector<AssetRecord> records);

        void apply(Scene& scene) override;
    };
}
