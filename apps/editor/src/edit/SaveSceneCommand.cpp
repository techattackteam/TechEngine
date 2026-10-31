#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <edit/EditSession.hpp>
#include <edit/SaveSceneCommand.hpp>

#include <cstddef>
#include <system_error>
#include <vector>

namespace TechEngine {
    SaveSceneCommand::SaveSceneCommand(EditSession& session, FileAccess& files, App& app, const AfterSave after) : m_session(&session), m_files(&files), m_app(&app), m_after(after) {
    }

    void SaveSceneCommand::apply(Scene& scene) {
        SceneFile file;
        const bool hasFile = m_session->getSceneFile(file);
        if (!hasFile) {
            TE_LOGGER_WARN("The scene was not saved: the editor did not open it from a file, so saving could overwrite one it never loaded");
            return;
        }

        std::vector<std::byte> bytes;
        SceneAsset{file.id}.write(scene, bytes);
        const std::error_code writeError = m_files->write(file.path, bytes);
        if (writeError) {
            TE_LOGGER_WARN("Failed to save the scene to {0}: {1}", file.path, writeError.message());
            return;
        }

        m_session->recordSave();
        TE_LOGGER_INFO("Saved the scene to {0}", file.path);
        if (m_after == AfterSave::Close) {
            m_app->requestStop();
        }
    }
}
