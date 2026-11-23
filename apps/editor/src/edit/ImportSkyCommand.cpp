#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/AssetNaming.hpp>
#include <edit/ImportSkyCommand.hpp>
#include <edit/RefreshAssetsCommand.hpp>

#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <utility>

namespace TechEngine {
    ImportSkyCommand::ImportSkyCommand(App& app, JobSystem& jobs, FileAccess& files, AssetIndex& index, std::vector<BatchId>& batches, SkyImportRequest request) : m_app(&app), m_jobs(&jobs), m_files(&files), m_index(&index), m_batches(&batches), m_request(std::move(request)) {
    }

    void ImportSkyCommand::apply(Scene&) {
        if (!m_request.name.empty() && !AssetNaming::isValidName(m_request.name)) {
            TE_LOGGER_WARN("A sky cannot be named '{0}', so nothing is created from {1}", m_request.name, m_request.sourcePath);
            return;
        }
        Task task = [app = m_app, files = m_files, index = m_index, request = std::move(m_request)] {
            SkyImportResult result;
            const TextureImporter importer(*files, &randomResourceId);
            const std::error_code importError = importer.importSky(request, result);
            if (importError) {
                TE_LOGGER_ERROR("Failed to import {0}: {1}", request.sourcePath, importError.message());
                return;
            }
            const std::string path = result.records[0].path;
            TE_LOGGER_INFO("Imported {0} into {1}", request.sourcePath, path);

            const std::error_code submitError = app->submitCommand(std::make_unique<RefreshAssetsCommand>(*files, *index));
            if (submitError) {
                TE_LOGGER_WARN("The simulation refused the refresh after {0}, so it is listed on the next refresh: {1}", path, submitError.message());
            }
        };
        m_batches->push_back(m_jobs->submit(std::span<Task>{&task, 1}));
    }
}
