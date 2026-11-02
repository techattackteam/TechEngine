#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/SimulationContext.hpp>

#include <edit/EditSession.hpp>
#include <edit/ExportCommand.hpp>
#include <project/Project.hpp>

#include <system_error>
#include <utility>

namespace TechEngine {
    ExportCommand::ExportCommand(const EditSession& session, const Project& project, const AssetIndex& index, const FileAccess& files, ComponentRegistry& registry, std::filesystem::path runtimePath, std::filesystem::path exportFolder)
        : m_session(&session), m_project(&project), m_index(&index), m_files(&files), m_registry(&registry), m_runtimePath(std::move(runtimePath)), m_exportFolder(std::move(exportFolder)) {
    }

    void ExportCommand::apply(Scene&) {
        if (m_session->hasUnsavedEdits()) {
            TE_LOGGER_WARN("The scene has unsaved edits, and the export does not include them: it exports the saved files");
        }
        const std::error_code exportError = m_project->exportTo(Role::Client, *m_index, *m_files, *m_registry, m_runtimePath, m_exportFolder);
        if (exportError) {
            TE_LOGGER_ERROR("Failed to export the project to {0}: {1}", m_exportFolder.string(), exportError.message());
        }
    }
}
