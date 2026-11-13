#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <edit/ImportModelCommand.hpp>

#include <algorithm>
#include <memory>
#include <span>
#include <system_error>
#include <utility>

namespace TechEngine {
    ImportModelCommand::ImportModelCommand(App& app, JobSystem& jobs, FileAccess& files, AssetIndex& index, std::vector<BatchId>& batches, ModelImportRequest request)
        : m_app(&app), m_jobs(&jobs), m_files(&files), m_index(&index), m_batches(&batches), m_request(std::move(request)) {
    }

    void ImportModelCommand::apply(Scene&) {
        Task task = [app = m_app, files = m_files, index = m_index, request = std::move(m_request)] {
            ModelImportResult result;
            const ModelImporter importer(*files, &randomResourceId);
            const std::error_code importError = importer.importFile(request, result);
            if (importError) {
                TE_LOGGER_ERROR("Failed to import {0}: {1}", request.sourcePath, importError.message());
                return;
            }
            const auto countOf = [&result](const StringId typeTag) {
                return std::ranges::count(result.records, typeTag, &AssetRecord::typeTag);
            };
            if (request.mode == ImportMode::MergedMesh) {
                TE_LOGGER_INFO("Imported {0} into {1} as one merged mesh, {2} materials and {3} textures", request.sourcePath, result.folder, countOf(MATERIAL_TYPE_TAG), countOf(TEXTURE_TYPE_TAG));
            } else {
                TE_LOGGER_INFO("Imported {0} into {1}: {2} nodes, {3} meshes, {4} materials and {5} textures", request.sourcePath, result.folder, result.nodeCount, countOf(MESH_TYPE_TAG), countOf(MATERIAL_TYPE_TAG), countOf(TEXTURE_TYPE_TAG));
            }

            const std::error_code submitError = app->submitCommand(std::make_unique<RegisterImportedAssetsCommand>(*index, std::move(result.records)));
            if (submitError) {
                TE_LOGGER_WARN("The simulation refused the index entries of {0}, so its files are indexed on the next launch: {1}", result.folder, submitError.message());
            }
        };
        m_batches->push_back(m_jobs->submit(std::span<Task>{&task, 1}));
    }

    RegisterImportedAssetsCommand::RegisterImportedAssetsCommand(AssetIndex& index, std::vector<AssetRecord> records) : m_index(&index), m_records(std::move(records)) {
    }

    void RegisterImportedAssetsCommand::apply(Scene&) {
        for (AssetRecord& record: m_records) {
            AssetRecord existing;
            const bool taken = m_index->find(record.id, existing);
            if (taken) {
                TE_LOGGER_ERROR("Imported {0} has the ID {1:#018x} of {2}, so it is not indexed", record.path, record.id.value(), existing.path);
                continue;
            }
            m_index->add(std::move(record));
        }
        m_records.clear();
    }
}
